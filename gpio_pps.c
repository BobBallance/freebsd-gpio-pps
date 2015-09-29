/* 
** Look at KASSERT(true, ("message"))
*/

#include <sys/cdefs.h>

#include "opt_platform.h"

#include <sys/param.h>

/* Temporary hack? */

/* Clang-style message 
#pragma message "Overriding the FREEBSD Version Number is a Bad Idea!"

#undef __FreeBSD_version
#define __FreeBSD_version 1100075	/* Master, propagated to newvers */
*/

#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/gpio.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/sysctl.h>

#include <sys/timepps.h>

#ifdef FDT
#include <dev/fdt/fdt_common.h>
 #include <dev/ofw/ofw_bus.h>
#endif

#include <dev/gpio/gpiobusvar.h>

#include <sys/rman.h>

#include "gpiobus_if.h"

#define	GPIOPPS_PIN	18
#define   GPIOPPS_NAME "gpio_pps"

#define GPIOPPS_LOCK(_sc)		mtx_lock(&(_sc)->sc_mtx)
#define	GPIOPPS_UNLOCK(_sc)		mtx_unlock(&(_sc)->sc_mtx)
#define GPIOPPS_LOCK_INIT(_sc) \
	mtx_init(&_sc->sc_mtx, device_get_nameunit(_sc->sc_dev), \
	    GPIOPPS_NAME, MTX_DEF)
#define GPIOPPS_LOCK_DESTROY(_sc)	mtx_destroy(&_sc->sc_mtx);

struct gpio_pps_softc 
{
  device_t	sc_dev;
  device_t	sc_busdev;
  struct mtx	sc_mtx;
  struct resource * sc_intr_resource;	/* interrupt resource -- needed to specify pin */
  int sc_irq_rid;


  device_t      sc_gpio_intr;
  void *sc_intr_cookie;		/* interrupt registration cookie */
  
  struct pps_state sc_pps;        /* sio.c needs a single pps_state */
  /* struct sysctl_oid	*sc_oid; */
  
};

static int gpio_pps_probe(device_t);
static int gpio_pps_attach(device_t);
static int gpio_pps_detach(device_t);


#ifdef PURE_MODULE
static int
gpio_pps_modevent(module_t mod __unused, int event, void* arg __unused)
{
  int error = 0;
  switch(event) {
  case MOD_LOAD:
    uprintf("gpio_pps loaded\n");
    break;

  case MOD_UNLOAD:
        uprintf("gpio_pps  unloaded\n");
        break;
  default:
    error = EOPNOTSUPP;
    break;
  }
  return error;
}
#endif

static void
gpio_pps_identify(driver_t *driver, device_t bus)
{
  device_t dev;
  
  device_printf(bus, "gpio_pps_identify called\n");
  dev = device_find_child(bus, GPIOPPS_NAME, -1);
  if (!dev) {
    dev = BUS_ADD_CHILD(bus, 0, GPIOPPS_NAME, -1);
    /*    KASSERT(dev, ("Unable to add gpio_pps to bus")); */
    if (dev) {
      device_printf(dev, "gpio_pps_identify: bus added\n");
    }
  }
}

static int
gpio_pps_probe(device_t dev)
{
	device_set_desc(dev, "GPIO pps");
        device_printf( dev, "gpio_pps_probe completes\n");  
  	return (0);
}

static int pps_count = 0;

static void
gpio_pps_intr(void * arg)
{
  pps_count++;
}

static int
gpio_pps_attach(device_t dev)
{
	struct gpio_pps_softc *sc;
        int error;
        
	sc = device_get_softc(dev);
	sc->sc_dev = dev;
	sc->sc_busdev = device_get_parent(dev);
        sc->sc_irq_rid = 0;
        sc->sc_gpio_intr = devclass_get_device(devclass_find("gpio"), 0);

        GPIOPPS_LOCK_INIT(sc);            

        if (!sc->sc_gpio_intr) {
          device_printf(dev, "Unable to find gpio_intr device...");
          return ENXIO;
        }
        
        device_printf(dev, "found the gpio device...");
        /*	int state; */

        GPIOPPS_LOCK(sc);

        if (BUS_CONFIG_INTR(sc->sc_gpio_intr, GPIOPPS_PIN, INTR_TRIGGER_EDGE, INTR_POLARITY_HIGH)) {
          GPIOPPS_UNLOCK(sc);
          device_printf(dev, "Unable to configure the interrupt...\n");
          return ENXIO;
        }

        /* Try with gpio object... Still strays? */
        sc->sc_intr_resource = bus_alloc_resource(sc->sc_gpio_intr,  SYS_RES_IRQ, &(sc->sc_irq_rid), 
                                                                               GPIOPPS_PIN, GPIOPPS_PIN, 1, RF_ACTIVE);

        if (!sc->sc_intr_resource) {
          GPIOPPS_UNLOCK(sc);          
          device_printf(dev, "Unable to create the interrupt resource.");
          return ENXIO;
        }

        if (rman_adjust_resource(sc->sc_intr_resource, GPIOPPS_PIN, GPIOPPS_PIN)) {
          uprintf("Adjustment failed\n");
          bus_release_resource(sc->sc_gpio_intr, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
          GPIOPPS_UNLOCK(sc);
          return ENXIO;
        }

        if (rman_get_start(sc->sc_intr_resource) != GPIOPPS_PIN) {
          uprintf("Adjustment failed -- start is wrong\n");
          bus_release_resource(sc->sc_gpio_intr, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
          GPIOPPS_UNLOCK(sc);
          return ENXIO;
        }


        error = bus_setup_intr(sc->sc_gpio_intr, sc->sc_intr_resource, INTR_TYPE_CLK, NULL,  gpio_pps_intr, sc, &(sc->sc_intr_cookie));

        if (error) {
          bus_release_resource(sc->sc_gpio_intr, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
          device_printf(dev, "Unable to hook interrupt handler\n");
          GPIOPPS_UNLOCK(sc);
          return (error);
        }

        if (bus_activate_resource(sc->sc_gpio_intr,  SYS_RES_IRQ,  sc->sc_irq_rid,  sc->sc_intr_resource)) {
          bus_release_resource(sc->sc_gpio_intr, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
          device_printf(dev, "Unable to activate interrupt handler\n");
          GPIOPPS_UNLOCK(sc);
          return ENXIO;
        }

        GPIOPPS_UNLOCK(sc);        
        device_printf(dev, "gpio_pps_attach completes\n");
	return (0);
}

static int
gpio_pps_detach(device_t dev)
{
	struct gpio_pps_softc *sc;

	sc = device_get_softc(dev);
        if (sc->sc_intr_resource) {
          GPIOPPS_LOCK(sc);
          bus_deactivate_resource(sc->sc_gpio_intr,  SYS_RES_IRQ,  sc->sc_irq_rid,  sc->sc_intr_resource);
          bus_teardown_intr(sc->sc_gpio_intr, sc->sc_intr_resource, sc->sc_intr_cookie);
          bus_release_resource(dev, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
          GPIOPPS_UNLOCK(sc);          
        }
	GPIOPPS_LOCK_DESTROY(sc);
        device_printf(dev, "gpio_pps_detach completes: count = %d\n", pps_count);        
        return 0;
}

static devclass_t gpio_pps_devclass;

#ifdef PURE_MODULE
static moduledata_t gpio_pps_mod = {
  GPIOPPS_NAME,
  gpio_pps_modevent,
  NULL
};


 DECLARE_MODULE(gpio_pps, gpio_pps_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
#endif

static device_method_t gpio_pps_methods[] = {
	/* Device interface */
        DEVMETHOD(device_identify,		gpio_pps_identify),  
	DEVMETHOD(device_probe,		gpio_pps_probe),
	DEVMETHOD(device_attach,	gpio_pps_attach),
	DEVMETHOD(device_detach,	gpio_pps_detach),

	{ 0, 0 }
};

static driver_t gpio_pps_driver = {
	GPIOPPS_NAME,
	gpio_pps_methods,
	sizeof(struct gpio_pps_softc),
};

/* This kills the kernel if you make it a child of gpio */
DRIVER_MODULE(gpio_pps,  gpiobus,   gpio_pps_driver, gpio_pps_devclass, 0, 0);
/* MODULE_DEPEND(gpio_pps, gpiobus, 1, 1, 1); */



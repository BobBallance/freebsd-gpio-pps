/* ----------------------------------------------------------------------------
 * Copyright (c) Bob Ballance 2015
 * 
 * gpio_pps.c -			Cr
 * 
 * gpio interrupt handler for PPS signals
 * 
 * ----------------------------------------------------------------------------
 */

#include <sys/cdefs.h>
#include "opt_platform.h"

#include <sys/param.h>

/* Clang-style message 
#pragma message "Overriding the FREEBSD Version Number is a Bad Idea!"

#undef __FreeBSD_version
#define __FreeBSD_version 1100075	
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
  phandle_t node, root, child;
  device_printf(bus, "gpio_pps_identify called\n");
  
  root = OF_finddevice("/");
  if (root == 0)
    return;

  for (node = OF_child(root); node != 0; node = OF_peer(node)) {

    if (!fdt_is_compatible_strict(node, "gpio-pps"))
      continue;

    /* Traverse the 'gpio-pps' node and add its children. */
    for (child = OF_child(node); child != 0; child = OF_peer(child)) {
      if (!OF_hasprop(child, "gpios"))
        continue;
      if (ofw_gpiobus_add_fdt_child(bus, driver->name, child) == NULL)
        printf("gpio_pps_identify: FDT child added\n");
        continue;
    }
  }
  device_printf(bus, "gpio_pps_identify: bus added\n");
}

static int
gpio_pps_probe(device_t dev)
{
	int match;
	phandle_t node;
	char *compat;

	/*
	 * We can match against our own node compatible string and also against
	 * our parent node compatible string.  The first is normally used to
	 * describe leds on a gpiobus and the later when there is a common node
	 * compatible with 'gpio-leds' which is used to concentrate all the
	 * leds nodes on the dts.
	 */
	match = 0;
	if (ofw_bus_is_compatible(dev, "gpio-pps"))
		match = 1;

	if (match == 0) {
          if ((node = ofw_bus_get_node(dev)) == -1) {
            device_printf(dev, "probe: no node\n");
            return (ENXIO);
          }
          if ((node = OF_parent(node)) == -1) {
            device_printf(dev, "probe: no parent node\n");            
            return (ENXIO);
          }
          if (OF_getprop_alloc(node, "compatible", 1,
                               (void **)&compat) == -1) {
            device_printf(dev, "probe: property\n");            
            return (ENXIO);
          }
          
          if (strcasecmp(compat, "gpio-pps") == 0)
            match = 1;
          free(compat, M_OFWPROP);
	}
        
	if (match == 0) {
          device_printf(dev, "probe: no match\n");
          return (ENXIO);
        }

	device_set_desc(dev, "GPIO PPS");
        device_printf(dev, "probe returns OK\n");
	return (0);
}

static int pps_count = 0;

static void
gpio_pps_intr(void * arg)
{
  struct gpio_pps_softc *sc = arg;  
  pps_capture(&(sc->sc_pps));
  pps_event(&(sc->sc_pps), PPS_CAPTUREASSERT);    
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

        pps_init_abi(&(sc->sc_pps));
        
        GPIOPPS_LOCK_INIT(sc);            

        GPIOPPS_LOCK(sc);

        /* Worked with gpio_intr. Try with busdev */
        if (BUS_CONFIG_INTR(sc->sc_dev, GPIOPPS_PIN, INTR_TRIGGER_EDGE, INTR_POLARITY_HIGH)) {
          GPIOPPS_UNLOCK(sc);
          device_printf(dev, "Unable to configure the interrupt...\n");
          return ENXIO;
        }

        /* Try with gpio object... Still strays? */
        sc->sc_intr_resource = bus_alloc_resource(sc->sc_dev,  SYS_RES_IRQ, &(sc->sc_irq_rid), 
                                                                               GPIOPPS_PIN, GPIOPPS_PIN, 1, RF_ACTIVE);

        if (!sc->sc_intr_resource) {
          GPIOPPS_UNLOCK(sc);          
          device_printf(dev, "Unable to create the interrupt resource.");
          return ENXIO;
        }

        if (rman_adjust_resource(sc->sc_intr_resource, GPIOPPS_PIN, GPIOPPS_PIN)) {
          uprintf("Adjustment failed\n");
          bus_release_resource(sc->sc_dev, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
          GPIOPPS_UNLOCK(sc);
          return ENXIO;
        }

        if (rman_get_start(sc->sc_intr_resource) != GPIOPPS_PIN) {
          uprintf("Adjustment failed -- start is wrong\n");
          bus_release_resource(sc->sc_dev, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
          GPIOPPS_UNLOCK(sc);
          return ENXIO;
        }

        error = bus_setup_intr(sc->sc_dev, sc->sc_intr_resource, INTR_TYPE_CLK, NULL,  gpio_pps_intr, sc, &(sc->sc_intr_cookie));
        if (error) {
          bus_release_resource(sc->sc_dev, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
          device_printf(dev, "Unable to hook interrupt handler\n");
          GPIOPPS_UNLOCK(sc);
          return (error);
        }

        if (bus_activate_resource(sc->sc_dev,  SYS_RES_IRQ,  sc->sc_irq_rid,  sc->sc_intr_resource)) {
          bus_release_resource(sc->sc_dev, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
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
          bus_deactivate_resource(sc->sc_dev,  SYS_RES_IRQ,  sc->sc_irq_rid,  sc->sc_intr_resource);
          bus_teardown_intr(sc->sc_dev, sc->sc_intr_resource, sc->sc_intr_cookie);
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



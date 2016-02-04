/* Copyright (c) 2016, Robert A. Ballance (ballance@swcp.com)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are met: 
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation 
*    and/or other materials provided with the distribution.

* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* The views and conclusions contained in the software and documentation are those
* of the authors and should not be interpreted as representing official policies,
* either expressed or implied, of the FreeBSD Project.
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
#include <sys/conf.h>

#include "gpiobus_if.h"

#define	GPIOPPS_PIN 4	
#define   GPIOPPS_NAME "gpiopps"
#define	PPS_NAME "pps"

#define GPIOPPS_LOCK(_sc)		mtx_lock(&(_sc)->sc_mtx)
#define	GPIOPPS_UNLOCK(_sc)		mtx_unlock(&(_sc)->sc_mtx)
#define GPIOPPS_LOCK_INIT(_sc) \
  mtx_init(&_sc->sc_mtx, device_get_nameunit(_sc->sc_dev),      \
           GPIOPPS_NAME, MTX_DEF)
#define GPIOPPS_LOCK_DESTROY(_sc)	mtx_destroy(&_sc->sc_mtx);

struct gpiopps_softc 
{
  device_t	sc_dev;
  device_t	sc_busdev;
  struct mtx	sc_mtx;
  struct resource * sc_intr_resource;	/* interrupt resource -- needed to specify pin */
  int sc_irq_rid;
  void *sc_intr_cookie;		/* interrupt registration cookie */
  
  struct pps_state sc_pps;        /* sio.c needs a single pps_state */
  struct cdev * sc_cdev;

  int sc_gpio_pin;
  /* struct sysctl_oid	*sc_oid; */
  
};

static int gpiopps_probe(device_t);
static int gpiopps_attach(device_t);
static int gpiopps_detach(device_t);

static	d_open_t	gpiopps_open;
static	d_close_t	gpiopps_close;
static	d_ioctl_t	        gpiopps_ioctl;


static struct cdevsw pps_cdevsw = {
	.d_version =	D_VERSION,
	.d_open =	gpiopps_open,
	.d_close =	gpiopps_close,
	.d_ioctl =	gpiopps_ioctl,
	.d_name =	PPS_NAME,
};



static void
gpiopps_identify(driver_t *driver, device_t bus)
{
  phandle_t node, root, child;
  device_printf(bus, "gpiopps_identify called\n");
  
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
        printf("gpiopps_identify: FDT child added\n");
        continue;
    }
  }
  device_printf(bus, "gpiopps_identify: bus added\n");
}

static int
gpiopps_probe(device_t dev)
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



#ifdef GPIOPPS_COUNT
static int pps_count = 0;
#endif

static void
gpiopps_intr(void * arg)
{
  struct gpiopps_softc *sc = arg;  
  pps_capture(&(sc->sc_pps));
  pps_event(&(sc->sc_pps), PPS_CAPTUREASSERT);    
#ifdef GPIOPPS_COUNT
  pps_count++;
#endif
}

static int
gpiopps_attach(device_t dev)
{
	struct gpiopps_softc *sc;
        int error;
        struct cdev* c_dev;
	sc = device_get_softc(dev);
	sc->sc_dev = dev;
	sc->sc_busdev = device_get_parent(dev);
        sc->sc_irq_rid = 0;

        
        sc->sc_pps.ppscap = PPS_CAPTUREASSERT;
        pps_init_abi(&(sc->sc_pps));
        GPIOPPS_LOCK_INIT(sc);            

        /* hint.gpiopps.0.pin=x */
        error = resource_int_value("gpiopps", 0,  "pin", &sc->sc_gpio_pin);
        if (error) {
          sc->sc_gpio_pin = GPIOPPS_PIN;
          device_printf(sc->sc_dev, "Device hint returned %s (%d)\n",
                        error == ENOENT ? "ENOENT" : "EFTYPE", error);
        }
        device_printf(sc->sc_dev, "Pin on GPIO %d\n", sc->sc_gpio_pin);
        
        GPIOPPS_LOCK(sc);


        if (BUS_CONFIG_INTR(sc->sc_dev, sc->sc_gpio_pin,  INTR_TRIGGER_EDGE, INTR_POLARITY_HIGH)) {
          GPIOPPS_UNLOCK(sc);
          device_printf(dev, "Unable to configure the interrupt...\n");
          return ENXIO;
        }

        /* Flags has to be just RF_ACTIVE for the adjustment to work */
        sc->sc_intr_resource = bus_alloc_resource(sc->sc_dev,  SYS_RES_IRQ, &(sc->sc_irq_rid), 
                                                  sc->sc_gpio_pin, sc->sc_gpio_pin, 1, RF_ACTIVE);

        if (!sc->sc_intr_resource) {
          GPIOPPS_UNLOCK(sc);          
          device_printf(dev, "Unable to create the interrupt resource.");
          return ENXIO;
        }

        if (rman_adjust_resource(sc->sc_intr_resource, sc->sc_gpio_pin, sc->sc_gpio_pin)) {
          uprintf("Adjustment failed\n");
          bus_release_resource(sc->sc_dev, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
          GPIOPPS_UNLOCK(sc);
          return ENXIO;
        }

        if (rman_get_start(sc->sc_intr_resource) != sc->sc_gpio_pin) {
          device_printf(sc->sc_dev, "Adjustment failed -- start is wrong\n");
          bus_release_resource(sc->sc_dev, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
          GPIOPPS_UNLOCK(sc);
          return ENXIO;
        }

        /* Jitter -- is it the GIANT lock, or not being a filter function? */
        error = bus_setup_intr(sc->sc_dev, sc->sc_intr_resource, 
                                               INTR_TYPE_CLK | INTR_MPSAFE, NULL,  gpiopps_intr, sc, &(sc->sc_intr_cookie));
        if (error) {
          bus_release_resource(sc->sc_dev, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
          device_printf(dev, "Unable to hook interrupt handler\n");
          GPIOPPS_UNLOCK(sc);
          return (error);
        }

	c_dev = make_dev(&pps_cdevsw, 0,    UID_ROOT, GID_WHEEL, 0600,  "pps%d", 0);

	sc->sc_cdev = c_dev;
	c_dev->si_drv1 = sc;
	c_dev->si_drv2 = (void*)0;
        GPIOPPS_UNLOCK(sc);        
        device_printf(dev, "gpiopps_attach completes\n");
	return (0);
}

static int
gpiopps_open(struct cdev* dev, int flags, int fmt, struct thread* td) 
{
  struct gpiopps_softc *sc = dev->si_drv1;
  GPIOPPS_LOCK(sc);
  if (bus_activate_resource(sc->sc_dev,  SYS_RES_IRQ,  sc->sc_irq_rid,  sc->sc_intr_resource)) {
    bus_release_resource(sc->sc_dev, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);
    device_printf(sc->sc_dev, "Unable to activate interrupt handler\n");
    GPIOPPS_UNLOCK(sc);
    return ENXIO;
  }
  GPIOPPS_UNLOCK(sc);
  return (0);
}

static int
gpiopps_close(struct cdev* dev, int flags, int fmt, struct thread* td) 
{
  struct gpiopps_softc *sc = dev->si_drv1;
  if (sc->sc_intr_resource) {
    GPIOPPS_LOCK(sc);
    bus_deactivate_resource(sc->sc_dev,  SYS_RES_IRQ,  sc->sc_irq_rid,  sc->sc_intr_resource);
    GPIOPPS_UNLOCK(sc);
  }
  return (0);
}

static int
gpiopps_ioctl(struct cdev *dev, u_long cmd, caddr_t data, int flags, struct thread *td)
{
  struct gpiopps_softc *sc = dev->si_drv1;
  int err;
  GPIOPPS_LOCK(sc);        
  err = pps_ioctl(cmd, data, &sc->sc_pps);
  GPIOPPS_UNLOCK(sc);        
  return (err);
}

static int
gpiopps_detach(device_t dev)
{
	struct gpiopps_softc *sc;

	sc = device_get_softc(dev);
        if (sc->sc_intr_resource) {
          GPIOPPS_LOCK(sc);

          bus_teardown_intr(sc->sc_dev, sc->sc_intr_resource, sc->sc_intr_cookie);
          bus_release_resource(dev, SYS_RES_IRQ, sc->sc_irq_rid, sc->sc_intr_resource);

          if (sc->sc_cdev) {
            destroy_dev(sc->sc_cdev);
          }
          GPIOPPS_UNLOCK(sc);          
        }
	GPIOPPS_LOCK_DESTROY(sc);

#ifdef GPIOPPS_COUNT
        device_printf(dev, "gpiopps_detach completes: count = %d\n", pps_count);
#else
        device_printf(dev, "gpiopps_detach completes\n");
#endif
        return 0;
}

static devclass_t gpiopps_devclass;


static device_method_t gpiopps_methods[] = {
	/* Device interface */
        DEVMETHOD(device_identify,		gpiopps_identify),  
	DEVMETHOD(device_probe,		gpiopps_probe),
	DEVMETHOD(device_attach,	gpiopps_attach),
	DEVMETHOD(device_detach,	gpiopps_detach),

	{ 0, 0 }
};

static driver_t gpiopps_driver = {
	GPIOPPS_NAME,
	gpiopps_methods,
	sizeof(struct gpiopps_softc),
};

/* This kills the kernel if you make it a child of gpio */
DRIVER_MODULE(gpiopps,  gpiobus,   gpiopps_driver, gpiopps_devclass, 0, 0);



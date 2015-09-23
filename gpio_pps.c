#include <sys/cdefs.h>

#include "opt_platform.h"
#include <sys/param.h>

/* Temporary hack? */
/* Clang-style message */
#pragma message "Overriding the FREEBSD Version Number is a Bad Idea!"

#undef __FreeBSD_version
#define __FreeBSD_version 1100075	/* Master, propagated to newvers */


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

#include <dev/fdt/fdt_common.h>
 #include <dev/ofw/ofw_bus.h>

#include <dev/gpio/gpiobusvar.h>

#include "gpiobus_if.h"


#define GPIOBL_LOCK(_sc)		mtx_lock(&(_sc)->sc_mtx)
#define	GPIOBL_UNLOCK(_sc)		mtx_unlock(&(_sc)->sc_mtx)
#define GPIOBL_LOCK_INIT(_sc) \
	mtx_init(&_sc->sc_mtx, device_get_nameunit(_sc->sc_dev), \
	    "gpiobacklight", MTX_DEF)
#define GPIOBL_LOCK_DESTROY(_sc)	mtx_destroy(&_sc->sc_mtx);

struct gpio_pps_softc 
{
	device_t	sc_dev;
	device_t	sc_busdev;
	struct mtx	sc_mtx;

	struct sysctl_oid	*sc_oid;
        /* sio.c needs a single pps_state */
  	struct	pps_state pps;

       /* these from parallel bus. Don't know if they are needed yet. */
	struct resource *intr_resource;	/* interrupt resource */
	void *intr_cookie;		/* interrupt registration cookie */

};

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

static moduledata_t gpio_pps_mod = {
  "gpio_pps",
  gpio_pps_modevent,
  NULL
};

DECLARE_MODULE(gpio_pps, gpio_pps_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
    

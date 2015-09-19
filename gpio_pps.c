/* Start by creating just a hello-world-type module */

#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

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
    

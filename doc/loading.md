# Loading the Kernel Module

Once you have built `gpiopps.ko`

* Copy `gpiopps.ko`  to your PI, and  use `kldload ./gpiopps.ko` to hand-test, or
* Copy  `gpiopps.ko`  to `/boot/modules/` on your PI, and change the `loader.conf` options to load the module at boot time.

In either case, messages listed by `dmesg` will show that the module is loaded, and `/dev/pps0` will be present in the `/dev/` directory. If the device `/dev/pps0` is present, reconfigure `ntpd` to use the reference clock, and restart `ntpd`. 

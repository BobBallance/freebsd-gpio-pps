# The GPIOPPS Module

`gpiopps` implements a simple FreeBSD kernel driver that intercepts Pulse-Per-Second signal on a GPIO interrupt and informs the kernel time mechanism that the signal has appeared.

By default, the module used GPIO PIN 4, but that setting can be changed by placing the following hint into the `/boot/loader.conf` file:

```
hint.gpiopps.0.pin=“N”
```

where N is the GPIO pin that you want to use.

## Configuring the Kernel to use `gpiopps`

The kernel configuration required two changes:

1. Enable the option `PPS_SYNC` in the kernel configuration file. I copied the default `RPI2` kernel configuration file to a new file, `RPI2_PPS` and made the change there. This meant changing the `KERNCONF` setting in two places:  the `crochet` setup file and in my Makefile.
2. Define the device in the device tree by adding a new stanza to the RPI2 device tree located at `…/sys/boot/fdt/dts/arm/rpi2.dts`

```
	pps {
			  compatible = “gpio-pps”;
				pps {
							label = “pps”;
							gpios = <&gpio 4 0>;
        };
   };
```

The string “gpio-pps” also appears in the device driver itself. This stanza is modeled on the one for `gpioleds`; some of the driver code is also modeled on that driver.

## The driver itself

Source code for the driver can be found on GitHub. It is licensed under the FreeBSD license. The source code defaults the GPIO pin to GPIO4, but as noted, this can be overridden by a hint defined in `/boot/loader.conf`. See the `crochet` notes for information about how to set up `/boot` to read `/boot/loader.conf`.


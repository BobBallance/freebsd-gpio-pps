# Configuring the Kernel to use `gpiopps`

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

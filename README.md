# A Stratum 1 Time Server using FreeBSD, Raspberry PI-2, and GPS

The core of this system is a GPS board that provides Pulse-per-Second (PPS)
time sychronization to a FreeBSD kernel module that uses a GPIO pin
for PPS input.

This solution has been tested on a Raspberry PI-2 using
an [Adafruit Ultimate GPS Breakout (version 3)](https://www.adafruit.com/products/746) and also
with the the [AdaFruit Ultimate GPS Hat](https://www.adafruit.com/products/2324). The
"Hat" provides a nicer footprint, but requires changes in the FreeBSD boot environment to work.

This repository contains a FreeBSD driver for crafting a
stratum-1 time server using:
* [FreeBSD 11.](https://www.freebsd.org),
* The [crochet](https://github.com/freebsd/crochet) build tool,
* A Raspberry PI-2, 
* A GPS board. that provides Pulse-Per-Second outputs. As noted, this has **only** been tested with a couple of Adafruit products.
* The u-boot-rpi FreeBSD port (see `/usr/ports/sysutils/u-boot-rpi2` in the FreeBSD ports tree
* A build environment for FreeBSD and the port.

The driver should be adaptable to any GPS hardware that provides a PPS output
compatible with the PI's 3.3v GPIO input.

The documentation is in this repository's [Wiki](https://github.com/BobBallance/freebsd-gpio-pps/wiki/Home) pages.

**Be sure to read through all the steps in the [FreeBSD-GPIO-PPS Documentation](https://github.com/BobBallance/freebsd-gpio-pps/wiki/Home)
before trying to use this approach!**

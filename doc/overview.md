# A Stratum-1 NTP Time Server using FreeBSD, A Raspberry PI 2 Model B, and Pulse-Per-Second Signalling over GPIO

## Overview

Having a Raspberry PI-2 and the Adafruit GPS board in-house, I got curious about how to build a FreeBSD kernel module that could use the PPS signal from the GPS to define a kernel-level time discipline. Other solutions to this problem that use the `gpsd` daemon running in user space to handle the time signal. The difference was that I wanted a kernel-level solution that was robust, and that would allow me to use the normal serial UART lines (Transmit (TX) and Receive (RX)) to provide access to the GPS signal. Reading the GPS was important for two reasons: debugging, and to handle parts like the Adafruit shield which automatically hooked up the serial lines.

In order to use the GPIO bank for the PPS input,  a FreeBSD device driver needs to register for the GPIO pin, create a new device (`/dev/pps0`), handle the PPS interrupt, and pass that information into the kernel timing process. Making the driver work required minor changes to the device tree, and also re-compiling the FreeBSD kernel for PPS signalling. 

[FreeBSD-11](https://www.freebsd.org) is supported on the [Raspberry Pi 2 Model B](https://www.raspberrypi.org/products/raspberry-pi-2-model-b/), and has long supported [kernel-level pulse-per-second (PPS) synchronization](http://docs.freebsd.org/doc/8.1-RELEASE/usr/share/doc/ntp/pps.html) within the kernel using various time sources. Historically, FreeBSD accepted the PPS signal via a serial line or a USB connection. This document shows how to extend the time sources to include a GPIO input.

Using the GPIO facilities on the PI has two benefits: simple wiring and the ability to use hardware solutions like the [Adafruit Ultimate GPS Breakout](https://www.adafruit.com/products/746) or the [AdaFruit Ultimate GPS Hat](https://www.adafruit.com/products/2324). With the “Hat” in place, I have a  local time server in the footprint of a regular PI that is also able to run network monitoring services, such as Nagios.

If you prefer to work in user-space, there’s a good description of how to hook up Linux, a GPS, and the `gpsd` daemon at [The Raspberry Pi as a Stratum-1 NTP Server](http://www.satsignal.eu/ntp/Raspberry-Pi-NTP.html#u-blox). That site used Linux, but the FreeBSD adaptation should be straightforward. It’s also a good site for NTP background.

## A Twist in the Road

Writing the driver and wiring in the GPS proved to be the easy part. The messiness arose from interactions among the GPS device, the on-board serial console (UART), and the `U-Boot` boot loader. 
Out-of-the-box (as of 12/2015), the boot loader appropriates the UART TX/RX lines on the PI for the serial console. If you attach those lines to the GPS, the boot loader becomes so mightly confused that it will not achieve its primary purpose (i.e. booting FreeBSD).
If you just want to verify that the board is working,  you can hook up the TX/RX lines *after* booting the system, and then use `gpsd` and the `cgps` commands to show you the serial output.
If you want to hard-wire the GPS serial lines, or use the “Hat” version of the Adafruit receiver that requires the UART, you’ll have to update the `U-Boot` boot loader to remove its use of the UART. Figuring out how to change the boot loader was the time-consuming  part; changing the options and recompiling the files on FreeBSD was then pretty simple.

## Hardware 

* Raspberry Pi 2 Model B. There are no particular dependencies on this particular hardware, other than FreeBSD support.
* Adafruit Ultimate GPS Breakout board. This board provides a 3.3v Pulse-per-Second signal that can be directly routed into a GPIO pin on the Raspberry PI. The board also provides normal NMEA GPS sentences using the TX/RX lines on the Raspberry PI. 
* (Optional) GPS Antenna and connector.
* (Optional) Breadboards, wire, etc.

## Software Toolchain

* [FreeBSD 11.0](https://www.freebsd.org) (Current). You want the current version for its support for the PI.
* [Crochet](https://github.com/freebsd/crochet), a tool for building embedded system images.
* [U-Boot](http://www.denx.de/wiki/U-Boot/WebHome), the boot loader used by FreeBSD. This code is available in the FreeBSD ports collection under `/usr/ports/sysutils/u-boot-rpi2`.
* NTPD, Version 4, as distributed with Free BSD.
* Compilers as needed by `crochet` and the `u-boot-rpi2` port

##  Step-by-Step Guide

The following notes assume that you already have FreeBSD running, and that you a basic knowledge of how to manage a FreeBSD installation! 

* Configure `crochet` to work in your current environment.
* Update the kernel configuration to support PPS signalling.
* Update the kernel device tree to define the `pps` device.
* Get the kernel driver. The source code is on GitHub. Thanks go out to Joseph Kong for [FreeBSD Device Drivers](https://www.nostarch.com/bsddrivers.htm) and lots of good documentation at the FreeBSD site.
* Cross-compile the driver in your `crochet` environment.
* [Wire in the GPS Receiver](wiring.html).
* Reboot with your new kernel and load the driver by hand.
* Update the `ntp.conf` file to use the local reference clock.
* Autoload the kernel module during boot.

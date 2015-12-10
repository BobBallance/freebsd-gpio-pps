# A Stratum 1 Time Server using FreeBSD, Raspberry PI-2, and GPS

The core of this system is a GPS board that provides Pulse-per-Second (PPS)
time sychronization to a FreeBSD kernel module that uses a GPIO pin
for PPS input.

This repository contains my notes and source code for crafting a
stratum-1 time server using

* FreeBSD 11.0 with the crochet build tool
* A Raspberry PI-2,
* An Adafruit Ultimate GPS Breakout Board that provides
  Pulse-Per-Second outputs.

The directions here can be adjusted to another Raspberry PI, and
should be adaptable to any GPS hardware that provides a PPS output
compatible with the PI's 3.3v GPIO input.

Be sure to read through all the steps before trying to use this
approach! 

To fully implement this solution, you will also need

* The FreeBSD source,
* The crochet build tool to build the image, and
* Source files for the U-boot boot files to manage console I/O during
booting.

## Hardware Requirements

This solution is tested on a Raspberry PI-2 using
an Adafruit Ultimate GPS Breakout Board (version 3). With minor
changes, it should be possible to use the Adafruit Utimate GPS Shield,
or any other GPS module that provides a Pulse-Per-Second output.

Wiring the System provides the details.

## Software Requirements

* FreeBSD source. This code is tested with FreeBSD 11.0 compile for
ARMv6.
* Crochet
* U-boot
* This kernel driver and configuration files.

## Changes to the FDT

## Changes to the boot environment

## Changes to the U-BOOT environment

##

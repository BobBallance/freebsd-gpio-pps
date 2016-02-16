# The GPIOPPS Module

`gpiopps` implements a simple FreeBSD kernel driver that intercepts Pulse-Per-Second signal on a GPIO interrupt and informs the kernel time mechanism that the signal has appeared.

By default, the module used GPIO PIN 4, but that setting can be changed by placing the following hint into the `/boot/loader.conf` file:

```
hint.gpiopps.0.pin=“N”
```

where N is the GPIO pin that you want to use.

## The driver itself

Source code for the driver can be found on GitHub. It is licensed under the FreeBSD license. The source code defaults the GPIO pin to GPIO4, but as noted, this can be overridden by a hint defined in `/boot/loader.conf`. See the `crochet` notes for information about how to set up `/boot` to read `/boot/loader.conf`.


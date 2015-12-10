# Setting up Crochet

“Crochet is a tool for building bootable FreeBSD images.” It is available from [Github](https://github.com/freebsd/crochet). I used `git` to download a copy into my FreeBSD environment, made the following configuration changes:

## The Crochet configuration file

`Crochet` requires you to define a base configuration. Here are the settings that I used. The `FREEBSD_SRC` variable is set to point to the source distribution being used for the  development.

```
board_setup RaspberryPi2
option GrowFS
option Ntpd
FREEBSD_SRC=<<path to the FreeBSD source that you intend to use>>
```

## In board/RaspberryPI2/setup.sh

The `board/RaspberryPI2/setup.sh` file contains additional configuration information for the board. If you will be using a new kernel configuration file (which I recommend), update the value for `KERNCONF`. My setup modified `setup.sh` as follows:

```
KERNCONF=RPI2_PPS
```

where `RPI2_PPS` names the kernel configuration file.

## Overlay Files

`Crochet` adds the files in the directory `…/board/RaspberryPi2/overlays` into your image. These files can overwrite the system defaults. The following files are updated in my configuration. You might want to change this list, or add to it!

`boot/loader.rc`
: **IMPORTANT**: this file has to be in place for the boot process to read `boot/loader.conf`. It was not there by default. I used the overlay mechanism to copy in the default `loader.rc`.

`boot/loader.conf`
: boot-time configuration settings, such as loading the `gpiopps` module, or setting up a wireless adapter.

`etc/ntp.conf`
: The NTP configuration file.

`etc/rc.conf`
: The FreeBSD startup controller. You’ll want to modify this to start NTPD on boot.

etc/ttys
: Modify etc/ttys to disable the FreeBSD serial console on `/dev/ttyu0`. I simply changed `ttyu0` status to `off`, as shown below.

```
	ttyu0  "/usr/libexec/getty 3wire.115200"  dialup  off secure
```



## Additional options that proved useful

* Tell `crochet` to create a new user for you.
* Use the overlay mechanism to reuse `ssh` keys for the host. Simply let the boot process complete once, get `ssh` running, and then copy over the keys from `/etc/ssh` into `…overlays/etc/ssh`. Ensure that the copied keys (in `…overlays/etc/ssh`) have the same permissions as the original, or `sshd` might refuse to use them.
* `Crochet` will copy  the ports tree into your new image. I used this option, but had to enlarge the  image size using the `imageSize` option.

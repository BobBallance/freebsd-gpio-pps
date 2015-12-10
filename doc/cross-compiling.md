# Cross-compiling for a Raspberry PI using FreeBSD and Crochet

`Crochet` hides most of the details of cross-compiling and building the image. I was left with figuring out a way to build and test a new kernel image. The new kernel module was to be developed outside the kernel source tree, since I wanted to be able to manage the code as an ordinary user, outside of the update process.

These directions will work for any module; but they specifically call out a module named `gpiopps`. Change the `Makefile` for your own use!

The setup uses *two* files: an outer shell script (called `mk`) that sets up some necessary paths for `make`, plus a `Makefile` that does the normal compilations. The outer script is needed to ensure that local `machine` directory is set correctly. Both reside in the same directory as the module source:

```
…/gpiopps/Makefile
…/gpiopps/mk
…/gpiopps/src/module.c
```

To configure the files, you need to know the kernel configuration name, and the full paths to (i) the FreeBSD source (to be compiled) and (ii) your crochet working directory. Note that the path to FreeBSD and the kernel configuration name also appear in the configuration files for `crochet`.

## The `mk` script

```
#!/usr/local/bin/bash

# The following have to be full paths to be compatible with
# where crochet likes to put generated files.
export FREEBSD_SRC="<<full path to FreeBSD src>>"
export CROCHET_DIR="<<full path to where you build using crochet>>"

# Make sure that machine/ points to the arm version
if [ ! -e machine ]; then
   ln -s $FREEBSD_SRC/sys/arm/include machine
fi

make 
```

## The Makefile

```
# Module Makefile, extended for gpiopps.
# The environment variables FREEBSD_SRC and CROCHET_DIR are set correctly.

# MODNAME is the name of the module
MODNAME=gpiopps

# KERNCONF is the name of the kernel configuration
KERNCONF=RPI2_PPS

XCOMPILE_TARGET=$(TARGET).$(TARGET_ARCH)
# Build the full path to the root of the crochet objects directory
CROCHET_OBJ=$(CROCHET_DIR)/work/obj/$(XCOMPILE_TARGET)/$(FREEBSD_SRC)/sys/$(KERNCONF)

KMOD = $(MODNAME)
SRCS = src/$(MODNAME).c

CFLAGS+= -I$(FREEBSD_SRC)/sys  -I$(CROCHET_OBJ)

.include <bsd.kmod.mk>
```

## Using the files

As `root`:

```
cd <<FreeBSD source directory>>
make buildenv TARGET_ARCH=armv6 BUILDENV_SHELL=<<your favorite shell>>

cd <<gpiopps directory>>
bash ./mk

```
If all goes well, the file `gpiopps.ko` will be generated in the `<<gpiopps directory>>`. 

You can now either

* Copy `gpiopps.ko`  to your PI, and  use `kldload ./gpiopps.ko` to hand-test, or
* Copy  `gpiopps.ko`  to `/boot/modules/` on your PI, and change the `loader.conf` options to load the module at boot time.

In either case, messages listed by `dmesg` will show that the moduleis loaded, and `/dev/pps0` will be present in the `/dev/` directory. If the device is present, reconfigure `ntpd` to use the reference clock, and restart `ntpd`. Notes on this process are found in `doc/ntp.md`.

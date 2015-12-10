# Retaking Control of the UART

The default boot loader appropriates the PI’s UART to be used as a serial console. If you want, or need, to hook up your GPS to the UART, you  have to recompile the boot loader. On FreeBSD, the port for the Raspberry PI 2 boot loader can be found in `/usr/ports/sysutils/u-boot-rpi2/`. The port is based on the [U-Boot](http://www.denx.de/wiki/U-Boot/) software base.

Disabling the UART during boot does not seem to be easily configurable at boot-time, but one of you may know better how to proceed. So far, I’ve figured out how to disable the serial console by compiling needed options directly into the loader. I alos keep two versions of the binary files handy in case I need to see the boot process in its entirety!

Two changes are needed to the source code:

1. Add definitions to *enable* silent booting in the core files. 
2. Add boot-time parameters to turn off use of the serial console.

Both changes are encapusulated as patch files. In the next two sections, I'll walk you through the process.

## Enabling Silent Boot

Following the instructions in `README.silent` (in the `docs` directory of the `U-Boot` port), you should enable the following options. Adding these options merely enables code in the loader. You need subsequent environment settings to actually disable the messages.

```
#define CONFIG_SILENT_CONSOLE
#define CONFIG_SYS_DEVICE_NULLDEV
#define CONFIG_SILENT_CONSOLE_UPDATE_ON_RELOC
#define CONFIG_SILENT_CONSOLE_UPDATE_ON_SET
```

To integrate this change into the ports environment, I created a patch file that the port system can apply automatically. This patch file must be added into the directory `/usr/ports/sysutils/u-boot-rpi2/files`.

```
*** include/configs/rpi_2.h.orig        Fri Nov 27 10:05:31 2015
--- include/configs/rpi_2.h     Fri Nov 27 10:04:15 2015
***************
*** 12,15 ****
--- 12,20 ----
  
  #include "rpi-common.h"
  
+ #define CONFIG_SILENT_CONSOLE
+ #define CONFIG_SYS_DEVICE_NULLDEV
+ #define CONFIG_SILENT_CONSOLE_UPDATE_ON_RELOC
+ #define CONFIG_SILENT_CONSOLE_UPDATE_ON_SET
+ 
  #endif
```

## The Boot-Time Options

While the full U-Boot system offers multiple dynamic configuration options, none of the ones that I tried were sufficient. Those tried included placing settings in `/boot/loader.conf`, in `config.txt`, and in `uEnv.tx`t (after compiling in a boot-time parameter to read `uEnv.txt`).

What *did* work was to add three lines to the `rpi_common.h` configuration file. Those three lines update the bootargs string that gets compiled into the binary file. Did I mention messy? The `rpi_common.h` file is itself patched as part of the port, so in the end I have generated a patch file for the patch file.

The three lines added are the equivalent of
```
stdin=nulldev
stdout=nulldev
silent=1
```
### Patching a Patch File

The patch file (for the patch file) looks like

```
% more patch_patch_include_configs_rpi__common.h 
*** patch-include_configs_rpi__common.h.orig    Thu Dec  3 04:22:15 2015
--- patch-include_configs_rpi__common.h Fri Nov 27 12:51:10 2015
***************
*** 1,6 ****
  --- include/configs/rpi-common.h.orig 2015-04-13 11:53:03.000000000 -0300
  +++ include/configs/rpi-common.h
! @@ -183,4 +183,63 @@
   
   #define CONFIG_BOOTDELAY 2
   
--- 1,6 ----
  --- include/configs/rpi-common.h.orig 2015-04-13 11:53:03.000000000 -0300
  +++ include/configs/rpi-common.h
! @@ -183,4 +183,66 @@
   
   #define CONFIG_BOOTDELAY 2
   
***************
*** 36,41 ****
--- 36,44 ----
  +     "pxefile_addr_r=0x00100000\0" \
  +     "kernel_addr_r=0x01000000\0" \
  +     "ramdisk_addr_r=0x02100000\0" \
+ +     "stdin=nulldev\0" \
+ +     "stdout=nulldev\0" \
+ +     "silent=1\0" \
  +     "Fatboot=" \
  +       "env exists loaderdev || env set loaderdev ${fatdev}; " \
  +       "env exists UserFatboot && run UserFatboot; " \
  ```

The lines prefixed by `+ +` are the new lines.

### Patching the patch file

1. Copy the patch file into the `files/` directory of the port.
2. Run the `patch` command to patch the port’s original patch file.
```
% patch patch-include_configs_rpi__common.h \ 
           patch_patch_include_configs_rpi__common.h
```
(That command is all on one line.)
3. Move the `patch_patch_include_configs_rpi__common.h` file to a safe place. Don’t leave it in the `files/` directory!
4. Now rebuild the port. The Makefile will automatically apply the patch files in `files/`, including the one for `configs/rpi_2.h`. 
5. Remember to install (or reinstall) the port. This will have thes side effect of restoring the uboot configuration file config.txt to its original state. If you have addional customizations, be sure to update your installed `config.txt`. By default, it can be found in `/usr/local/share/u-boot/u-boot-rpi2/`.

## Using the rebuilt files

Crochet will automatically pick up the files from the installed port. Once your PI boots, the files will be found on the PI in `/boot/msdos`. You can edit `config.txt` there, or even replace the boot loader files with ones that use the UART.

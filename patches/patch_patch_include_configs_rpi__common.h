*** patch-include_configs_rpi__common.h.orig	Thu Dec  3 04:22:15 2015
--- patch-include_configs_rpi__common.h	Fri Nov 27 12:51:10 2015
***************
*** 1,6 ****
  --- include/configs/rpi-common.h.orig	2015-04-13 11:53:03.000000000 -0300
  +++ include/configs/rpi-common.h
! @@ -183,4 +183,63 @@
   
   #define CONFIG_BOOTDELAY 2
   
--- 1,6 ----
  --- include/configs/rpi-common.h.orig	2015-04-13 11:53:03.000000000 -0300
  +++ include/configs/rpi-common.h
! @@ -183,4 +183,66 @@
   
   #define CONFIG_BOOTDELAY 2
   
***************
*** 36,41 ****
--- 36,44 ----
  +	"pxefile_addr_r=0x00100000\0" \
  +	"kernel_addr_r=0x01000000\0" \
  +	"ramdisk_addr_r=0x02100000\0" \
+ +	"stdin=nulldev\0" \
+ +	"stdout=nulldev\0" \
+ +	"silent=1\0" \
  +	"Fatboot=" \
  +	  "env exists loaderdev || env set loaderdev ${fatdev}; " \
  +	  "env exists UserFatboot && run UserFatboot; " \

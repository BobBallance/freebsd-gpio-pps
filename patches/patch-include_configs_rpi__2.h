*** include/configs/rpi_2.h.orig	Tue Nov 24 10:33:44 2015
--- include/configs/rpi_2.h	Tue Nov 24 10:35:34 2015
***************
*** 10,15 ****
--- 10,20 ----
  #define CONFIG_SKIP_LOWLEVEL_INIT
  #define CONFIG_BCM2836
  
+ /* Force silent boot... */
+ #define CONFIG_SILENT_CONSOLE
+ #define CONFIG_SYS_DEVICE_NULLDEV
+ #define CONFIG_SILENT_CONSOLE_UPDATE_ON_RELOC
+ 
  #include "rpi-common.h"
  
  #endif

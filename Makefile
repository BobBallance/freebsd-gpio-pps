#
# For cross-compiling --- we need to set up some different paths.
#
# What this makefile does *not* do is to change the .mk include paths. You have to do that from an environment
# variable (MAKESYSPATH) or the -m flag for the make command.
#


KERNCONF = RPI2_DEV
XCOMPILE_TARGET=arm.armv6
CROCHET_OBJ=$(CROCHET_DIR)/work/obj/$(XCOMPILE_TARGET)/$(FREEBSD_SRC)/sys/$(KERNCONF)

KMOD = gpiopps
SRCS = gpiopps.c

CFLAGS+= -I$(FREEBSD_SRC)/sys  -I$(CROCHET_OBJ)

.include <bsd.kmod.mk>


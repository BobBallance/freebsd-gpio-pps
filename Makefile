# Generiic simple makefile

KMOD = gpio_pps
SRCS = gpio_pps.c

.include <bsd.kmod.mk>

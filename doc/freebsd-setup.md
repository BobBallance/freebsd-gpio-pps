# Compiling Free BSD for the Raspberry PI

I maintain separate source trees for FreeBSD on the server, and FreeBSD for the PI. This approach cleanly separates the development environments and settings between my “production” environment where the cross-compiling took place, and the “development” environment where I was building PI images.

The instructions for [synchronizing the FreeBSD kernel sources](https://www.freebsd.org/doc/handbook/synching.html) provide a good introduction.

The `crochet` tool was used to build the PI images.


#!/usr/local/bin/bash
#
# run as root...
ARCH=armv6
DIR=`pwd`
BSD_DIR=$HOME/projects/freebsd/src
WORKDIR=~$DIR/work

mkdir $WORKDIR

cd $BSD_DIR
make kernel-toolchain TARGET_ARCH=$ARCH >& $WORKDIR/toolchain.log
make buildenv TARGET_ARCH=$ARCH BUILDENV_SHELL=/usr/local/bin/bash >& $WORKDIR/toolchain.log
cd $DIR
make


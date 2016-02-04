#!/usr/local/bin/bash

TOP="/usr/home/bob"
export FREEBSD_SRC="$TOP/projects/freebsd/src"
export CROCHET_DIR="$TOP/projects/freebsd/crochet"

if [ ! -e machine ]; then
   ln -s $FREEBSD_SRC/sys/arm/include machine
fi

make 

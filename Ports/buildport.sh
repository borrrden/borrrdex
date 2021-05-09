#!/bin/bash -e

export JOBCOUNT=$(nproc)

if [ -z "$BORRRDEX_SYSROOT" ]; then
    export BORRRDEX_SYSROOT=$HOME/.local/share/borrrdex/sysroot

    echo "warning: BORRRDEX_SYSROOT not set. Automatically set to $BORRRDEX_SYSROOT"
fi

export CC=borrrdex-clang
export CXX=borrrdex-clang++
export CFLAGS=-Wno-error
export BORRRDEX_PREFIX=/system

. ./$1.sh

if [ "$2" != build ]; then
    unpack 
fi

if [ "$2" != unpack ]; then
    buildp
fi
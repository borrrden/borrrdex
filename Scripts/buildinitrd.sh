#!/bin/bash

SPATH=$(dirname $(readlink -f "$0"))
if [ -z "$BORRRDEX_SYSROOT" ]; then
    export BORRRDEX_SYSROOT=$HOME/.local/share/borrrdex/sysroot
fi

pushd $SPATH/..

mkdir -p Initrd

cp Resources/* Initrd/
cp -L $BORRRDEX_SYSROOT/system/lib/libc.so* \
    $BORRRDEX_SYSROOT/system/lib/libc++*.so* \
    $BORRRDEX_SYSROOT/system/lib/libunwind.so* \
    $BORRRDEX_SYSROOT/system/lib/ld.so* \
    $BORRRDEX_SYSROOT/system/lib/libfreetype.so* \
    $BORRRDEX_SYSROOT/system/lib/libpthread.so* \
    $BORRRDEX_SYSROOT/system/lib/librt.so* \
    $BORRRDEX_SYSROOT/system/lib/libdl.so* \
    Initrd/

nm Kernel/build/kernel.elf > Initrd/kernel.map

cd Initrd/
tar -cf ../initrd.tar *
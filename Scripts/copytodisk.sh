#!/bin/bash -e

if [[ "$1" != "nomount" ]]; then
    LOOPBACK_DEVICE=""
    DEVICE=""

    sudo mkdir -p /mnt/borrrdex

    if [ -z $1 ]; then
        LOOPBACK_DEVICE=$(sudo losetup --find --partscan --show Disks/Borrrdex.img)
        DEVICE="${LOOPBACK_DEVICE}"p2
    else
        DEVICE=$1
    fi

    echo "Mounting ${DEVICE} on /mnt/borrrdex..."
    sudo mount $DEVICE /mnt/borrrdex
    sudo chmod -R 777 /mnt/borrrdex
fi

cp initrd.tar /mnt/borrrdex/borrrdex/initrd.tar
cp Kernel/build/kernel.elf /mnt/borrrdex/borrrdex/kernel.elf
cp -ru $HOME/.local/share/borrrdex/sysroot/system/* /mnt/borrrdex
cp -ru Base/* /mnt/borrrdex

if [[ "$1" != "nomount" ]]; then
    echo "Unmounting /mnt/borrrdex..."
    sudo umount /mnt/borrrdex

    if [ -z $1 ]; then
        sudo losetup -d ${LOOPBACK_DEVICE}
    fi

    sudo rmdir /mnt/borrrdex
fi

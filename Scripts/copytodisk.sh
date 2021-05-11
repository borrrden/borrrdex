#!/bin/bash -e

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

sudo cp initrd.tar /mnt/borrrdex/borrrdex/initrd.tar
sudo cp Kernel/build/kernel.elf /mnt/borrrdex/borrrdex/kernel.elf
sudo cp -ru $HOME/.local/share/borrrdex/sysroot/system/* /mnt/borrrdex
sudo cp -ru Base/* /mnt/borrrdex

echo "Unmounting /mnt/borrrdex..."
sudo umount /mnt/borrrdex

if [ -z $1 ]; then
    sudo losetup -d ${LOOPBACK_DEVICE}
fi

sudo rmdir /mnt/borrrdex
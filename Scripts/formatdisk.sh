#!/bin/bash -e

function _unmount() {
    umount /mnt/borrrdex
    umount /mnt/borrrdexEFI
}

function _cleanup() {
    losetup -d "${LOOPBACK_DEVICE}"
    rmdir /mnt/borrrdex
    rmdir /mnt/borrrdexEFI
}

LOOPBACK_DEVICE=$(losetup --find --partscan --show Disks/Borrrdex.img)
echo "Mounted image as loopback device at ${LOOPBACK_DEVICE}"

echo "Formatting ${LOOPBACK_DEVICE}p2 as ext2"
mkfs.ext2 -b 4096 "${LOOPBACK_DEVICE}"p2

echo "Formatting ${LOOPBACK_DEVICE}p3 as FAT32"
mkfs.vfat -F 32 "${LOOPBACK_DEVICE}"p3

mkdir -p /mnt/borrrdex
mkdir -p /mnt/borrrdexEFI
mount "${LOOPBACK_DEVICE}"p2 /mnt/borrrdex
mount "${LOOPBACK_DEVICE}"p3 /mnt/borrrdexEFI

mkdir -p /mnt/borrrdexEFI/EFI/BOOT
if [ -e "$HOME/.local/share/borrrdex/share/limine/BOOTX64.EFI" ]; then
    # Check if installed
    cp "$HOME/.local/share/borrrdex/share/limine/BOOTX64.EFI" /mnt/borrrdexEFI/EFI/BOOT
    cp "$HOME/.local/share/borrrdex/share/limine/limine.sys" /mnt/borrrdex
elif [ -e "Toolchain/limine-2.0-bin/BOOTX64.EFI" ]; then
    # Check for limine built during the toolchain stage
    cp Toolchain/limine-2.0-bin/BOOTX64.EFI /mnt/borrrdexEFI/EFI/BOOT
    cp Toolchain/limine-2.0-bin/limine.sys /mnt/borrrdex/
else
    echo "Failed to find limine BOOTX64.EFI or limine.sys"
    _unmount
    _cleanup
    exit 1
fi

mkdir -p /mnt/borrrdex/borrrdex/boot

_unmount

if [ -x "$(command -v limine-install)" ]; then
    limine-install "${LOOPBACK_DEVICE}" 1
else
    export PATH=$PATH:$HOME/.local/share/borrrdex/bin
    if [ -x "$(command -v limine-install)" ]; then
        limine-install "${LOOPBACK_DEVICE}" 1
    elif [ -e "Toolchain/limine-2.0-bin/limine-install" ]; then
        Toolchain/limine-2.0-bin/limine-install "${LOOPBACK_DEVICE}" 1
    else
        echo "Failed to find limine-install!"
        _cleanup
        exit 1
    fi
fi

_cleanup
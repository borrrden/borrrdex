#!/bin/bash -e

SPATH=$(dirname $(readlink -f "$0"))

pushd $SPATH/..
export BORRRDEXDIR=$(pwd)

mkdir -p Disks

dd if=/dev/zero of=Disks/Borrrdex.img bs=512 count=2097152
sfdisk Disks/Borrrdex.img < Scripts/partitions.sfdisk

echo "Formatting disk!"

if [ -z "$TOOLCHAIN_PREFIX" ]; then
    export TOOLCHAIN_PREFIX=$HOME/.local/share/borrrdex
fi

sudo Scripts/formatdisk.sh
#!/bin/bash -e

SPATH=$(dirname $(readlink -f "$0"))

pushd $SPATH/..
export BORRRDEXDIR=$(pwd)

if ! [ -x "$(command -v borrrdex-clang)" ]; then
	echo "Borrrdex cross toolchain not found (Did you forget to build toolchain? Or is it just not in PATH?)"
	exit 1
fi

cd $BORRRDEXDIR/LibC
meson build --cross $SPATH/borrrdex.cross-file

cd $BORRRDEXDIR
make libc
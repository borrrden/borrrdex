#!/bin/bash -ex

SPATH=$(dirname $(readlink -f "$0"))

if [ -z "$BORRRDEX_SYSROOT" ]; then
    export BORRRDEX_SYSROOT=$HOME/.local/share/borrrdex/sysroot
fi

export PATH="$HOME/.local/share/borrrdex/bin:$PATH"

ln -sfT ../../../include/c++ $HOME/.local/share/borrrdex/sysroot/system/include/c++
cp $HOME/.local/share/borrrdex/lib/x86_64-borrrdex/c++/*.so* $HOME/.local/share/borrrdex/sysroot/system/lib

pushd $SPATH
$SPATH/libc.sh

cd $SPATH/..
export BORRRDEXDIR=$(pwd)

if ! [ -x "$(command -v borrrdex-clang)" ]; then
    echo "Borrrdex cross toolchain not found (Did you forget to build toolchain?)"
    exit 1
fi

mkdir -p LibBor/build
cd LibBor/build
cmake -G Ninja .. -DCMAKE_TOOLCHAIN_FILE=../../Scripts/borrrdex-cmake-options.txt -DCMAKE_INSTALL_PREFIX=$BORRRDEX_SYSROOT

mkdir -p ../../System/build
cd ../../System/build
cmake -G Ninja .. -DCMAKE_TOOLCHAIN_FILE=../../Scripts/borrrdex-cmake-options.txt

mkdir -p ../../Kernel/build
cd ../../Kernel/build
cp $SPATH/lai-CMakeLists.txt ../src/lai/CMakeLists.txt
cmake -G Ninja .. -DCMAKE_TOOLCHAIN_FILE=../../Scripts/borrrdex-cmake-options.txt

cd ../../Ports
./buildport.sh zlib
./buildport.sh libpng
./buildport.sh freetype
./buildport.sh libressl
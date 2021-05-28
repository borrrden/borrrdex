#!/bin/bash -e

SPATH=$(dirname $(readlink -f "$0"))

cd $SPATH/..
export BORRRDEXDIR=$(pwd)
cd $SPATH

export BINUTILS_SRC_DIR=binutils-2.32
export LLVM_SRC_DIR=llvm-project
export LIMINE_SRC_DIR=limine-2.0-bin

if [ -z "$JOBCOUNT" ]; then
    export JOBCOUNT=$(nproc)
fi

_unpack_binutils() {
    curl -L "http://ftpmirror.gnu.org/binutils/binutils-2.32.tar.gz" -o binutils-2.32.tar.gz
    tar -xzf binutils-2.32.tar.gz
    rm binutils-2.32.tar.gz

    pushd $BINUTILS_SRC_DIR
    patch -p1 < ../borrrdex-binutils-2.32.patch
    cd ld
    aclocal
    automake
    autoreconf
    popd
}

_unpack_llvm() {
    git clone https://github.com/borrrden/llvm-project --depth 1 --branch release/11.x $LLVM_SRC_DIR
}

_unpack_limine(){
    git clone https://github.com/limine-bootloader/limine --branch=v2.0-branch-binary $LIMINE_SRC_DIR --depth 1
}

_build_binutils() {
    pushd $BINUTILS_SRC_DIR

    ./configure --target=x86_64-borrrdex --prefix=$TOOLCHAIN_PREFIX --with-sysroot=$BORRRDEX_SYSROOT --disable-werror --enable-shared

    make -j $JOBCOUNT
    make install
    popd
}

_build_llvm() {
    if [ -z "$LINKCOUNT" ]; then
        export LINKCOUNT=4
        echo "Linking llvm will use a lot of memory, if you run out of memory try reducing the amount of link jobs by setting \$LINKCOUNT to a lower value (automatically set to $LINKCOUNT)."
        echo "Press Any key to continue..."
        read
    fi

    pushd $LLVM_SRC_DIR
    mkdir -p build
    cd build
    cmake -C ../clang/cmake/caches/Borrrdex.cmake -DCMAKE_INSTALL_PREFIX=$TOOLCHAIN_PREFIX -DDEFAULT_SYSROOT=$BORRRDEX_SYSROOT -DLLVM_PARALLEL_LINK_JOBS=$LINKCOUNT ../llvm -G Ninja

    ninja -j $JOBCOUNT
    ninja install

    ln -sf clang $TOOLCHAIN_PREFIX/bin/borrrdex-clang
    ln -sf clang++ $TOOLCHAIN_PREFIX/bin/borrrdex-clang++
}

_build_limine(){
    pushd $LIMINE_SRC_DIR

    make
    make install PREFIX=$TOOLCHAIN_PREFIX
    popd
}

_prepare() {
    mkdir -p $BORRRDEX_SYSROOT/system/{include,lib,bin}
    pushd $BORRRDEX_SYSROOT/..| 
    curl -L https://github.com/borrrden/borrrdex/releases/download/0.1/sysroot.tar.xz | tar -zxf -
    popd
}

_binutils() {
    if [ ! -d "$BINUTILS_SRC_DIR" ]; then
        _unpack_binutils
    fi

    _build_binutils
}

_llvm() {
    if [ ! -d "$LLVM_SRC_DIR" ]; then
        _unpack_llvm
    fi

    _build_llvm
}

_limine(){
    if [ ! -d "$LIMINE_SRC_DIR" ]; then
        _unpack_limine
    fi
    
    _build_limine
}

_build() {
    _prepare
	cd $SPATH
	_binutils
	cd $SPATH
	_llvm
	cd $SPATH
	_limine
	cd $SPATH
	
	echo "Binutils, LLVM and limine have been built."
}

if [ -z "$BORRRDEX_SYSROOT" -o -z "$TOOLCHAIN_PREFIX" ]; then
    export TOOLCHAIN_PREFIX=$HOME/.local/share/borrrdex
    export BORRRDEX_SYSROOT=$HOME/.local/share/borrrdex/sysroot
    echo "BORRRDEX_SYSROOT or TOOLCHAIN_PREFIX not set, using defaults: \
    TOOLCHAIN_PREFIX: $TOOLCHAIN_PREFIX\nBORRRDEX_SYSROOT: $BORRRDEX_SYSROOT"
fi

if [ -z "$1" ]; then
    echo "Usage: $0 (clean/prepare/binutils/llvm/limine/build)"
else
    pushd $SPATH
    _$1
fi
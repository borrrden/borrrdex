#!/bin/bash -e

export BUILD_DIR=libpng-1.6.37

unpack(){
    wget "https://download.sourceforge.net/libpng/libpng-1.6.37.tar.gz"
    tar -xzvf libpng-1.6.37.tar.gz
    rm libpng-1.6.37.tar.gz
}
 
buildp(){
    cd $BUILD_DIR
    patch -p1 < ../borrrdex-libpng-1.6.37.patch
    ./configure --prefix=$BORRRDEX_PREFIX --host=x86_64-borrrdex --enable-shared
    make -j$JOBCOUNT
    make install DESTDIR=$BORRRDEX_SYSROOT
}
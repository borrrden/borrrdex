#!/bin/bash -e

export BUILD_DIR=freetype-2.10.4

unpack(){
    wget "https://download.savannah.gnu.org/releases/freetype/freetype-2.10.4.tar.gz"
    tar -xzvf freetype-2.10.4.tar.gz
    rm freetype-2.10.4.tar.gz
}
 
buildp(){
    cd $BUILD_DIR
    patch -p1 < ../borrrdex-freetype-2.10.4.patch
    ./configure --host=x86_64-borrrdex --prefix=$BORRRDEX_PREFIX --with-harfbuzz=no --with-bzip2=no --disable-mmap --with-zlib=no --with-png=no --enable-shared --with-brotli=no
    make -j $JOBCOUNT
    make install DESTDIR=$BORRRDEX_SYSROOT
    ln -sf freetype2/freetype $BORRRDEX_SYSROOT/$BORRRDEX_PREFIX/include/
    ln -sf freetype2/ft2build.h $BORRRDEX_SYSROOT/$BORRRDEX_PREFIX/include/
}
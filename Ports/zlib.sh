export BUILD_DIR=zlib-1.2.11

unpack(){
    wget "https://zlib.net/zlib-1.2.11.tar.gz"
    tar -xzvf zlib-1.2.11.tar.gz
    rm zlib-1.2.11.tar.gz
}
 
buildp(){
    cd $BUILD_DIR
    ./configure --prefix=$BORRRDEX_PREFIX
    make -j$JOBCOUNT
    make install DESTDIR=$BORRRDEX_SYSROOT
}
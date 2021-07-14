export BUILD_DIR=zlib-1.2.11

unpack(){
    wget -O zsh.tar.xz "https://sourceforge.net/projects/zsh/files/zsh/5.8/zsh-5.8.tar.xz/download"
    tar -xvf zsh.tar.xz
 	export BUILD_DIR=zsh-5.8
    rm zsh.tar.xz
}
 
buildp(){
    cd $BUILD_DIR
    patch -p1 < ../borrrdex-zsh-5.8.patch
    ./configure --prefix=$BORRRDEX_PREFIX --host=x86_64-borrrdex
    make -j$JOBCOUNT
    make install DESTDIR=$BORRRDEX_SYSROOT
}
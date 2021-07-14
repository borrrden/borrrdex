unpack(){
 	wget "https://ftp.gnu.org/pub/gnu/ncurses/ncurses-6.2.tar.gz"
 	tar -xzvf ncurses-6.2.tar.gz
 	export BUILD_DIR=ncurses-6.2
 	rm ncurses-6.2.tar.gz
}
 
buildp(){
 	cd $BUILD_DIR
 	patch -p1 < ../borrrdex-ncurses-6.2.patch
 	./configure --prefix=$BORRRDEX_PREFIX --host=x86_64-borrrdex --without-ada --without-shared --without-cxx-shared
	ln -sf $BORRRDEX_SYSROOT/system/include/ncurses/* $BORRRDEX_SYSROOT/system/include
 	make -j$JOBCOUNT
 	make install DESTDIR=$BORRRDEX_SYSROOT
}


if [ -z $1 ]
then
	echo usage: $0 release_name "(as in 0.2.1alpha)"
	exit 1
fi

#FOX: `configure --with-opengl=no --disable-jpeg --disable-png --disable-tiff --disable-zlib --disable-bz2lib` and `make install`
#AUDIOFILE: apply any of my patches to audiofile and `configure` and `make install`
#cd to source directory


dir=rezound-bin-x86-linux-$1

mkdir $dir
configure --prefix=`pwd`/$dir --enable-standalone --with-audiofile-include=/usr/local/include --with-audiofile-path=/usr/local/lib --with-FOX-include=/usr/local/include --with-FOX-path=/usr/local/lib

make clean
make
make install-strip

tar -cvzf ~/$dir.tar.gz $dir

rm -rf $dir


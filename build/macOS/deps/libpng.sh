#!/bin/bash -e

. sdk.sh
PNG_VERSION=1.6.38

if [ ! -d libpng-src ]; then
	wget https://download.sourceforge.net/libpng/libpng-$PNG_VERSION.tar.gz
	tar -xzvf libpng-$PNG_VERSION.tar.gz
	mv libpng-$PNG_VERSION libpng-src
	rm libpng-$PNG_VERSION.tar.gz
fi

rm -rf libpng

cd libpng-src

for ARCH in x86_64 arm64
do
	echo "Building libpng for $ARCH"
	mkdir -p build; cd build
	cmake .. \
		-DCMAKE_BUILD_TYPE=Release \
		-DPNG_SHARED=OFF \
		-DPNG_TESTS=OFF \
		-DPNG_EXECUTABLES=OFF \
		-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_OSVER \
		-DCMAKE_OSX_ARCHITECTURES=$ARCH
	cmake --build . -j
	if [ $ARCH = "x86_64" ]; then
		make DESTDIR=$PWD/../../libpng install
		mv ../../libpng/usr/local/include ../../libpng/include
		mv ../../libpng/usr/local/lib/libpng16.a ../../libpng/templib_$ARCH.a
		rm -rf ../../libpng/usr
	else
		mv libpng16.a ../../libpng/templib_$ARCH.a
	fi
	cd ..; rm -rf build
done

# repack into one .a
cd ../libpng
lipo -create templib_*.a -output libpng.a
rm templib_*.a

echo "libpng build successful"

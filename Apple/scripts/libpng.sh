#!/bin/bash -e

PNG_VERSION=1.6.39

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d libpng-src ]; then
	wget https://download.sourceforge.net/libpng/libpng-$PNG_VERSION.tar.gz
	tar -xzvf libpng-$PNG_VERSION.tar.gz
	mv libpng-$PNG_VERSION libpng-src
	rm libpng-$PNG_VERSION.tar.gz
fi

rm -rf libpng

mkdir -p libpng/include

cd libpng-src

for ARCH in x86_64 arm64
do
	echo "Building libpng for $ARCH"
	mkdir -p build; cd build
	cmake .. \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_ASM_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_INSTALL_PREFIX="." \
		-DCMAKE_OSX_ARCHITECTURES=$ARCH \
		-DPNG_SHARED=OFF \
		-DPNG_TESTS=OFF \
		-DPNG_EXECUTABLES=OFF

	cmake --build . -j
	make install -s

	if [ $ARCH = "x86_64" ]; then
		cp -rv include ../../libpng
		cp -v lib/libpng16.a ../../libpng/templib_$ARCH.a
	else
		cp -v lib/libpng16.a ../../libpng/templib_$ARCH.a
	fi

	cd ..; rm -rf build
done

# repack into one .a
cd ../libpng
lipo -create templib_*.a -output libpng.a
rm templib_*.a

echo "libpng build successful"

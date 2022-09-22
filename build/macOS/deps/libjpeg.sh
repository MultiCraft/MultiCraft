#!/bin/bash -e

. sdk.sh
JPEG_VERSION=2.1.4

if [ ! -d libjpeg-src ]; then
	wget https://download.sourceforge.net/libjpeg-turbo/libjpeg-turbo-$JPEG_VERSION.tar.gz
	tar -xzvf libjpeg-turbo-$JPEG_VERSION.tar.gz
	mv libjpeg-turbo-$JPEG_VERSION libjpeg-src
	rm libjpeg-turbo-$JPEG_VERSION.tar.gz
fi

rm -rf ../../libjpeg

cd libjpeg-src

for ARCH in x86_64 arm64
do
	echo "Building libjpeg for $ARCH"
	mkdir -p build; cd build
	cmake .. \
		-DCMAKE_BUILD_TYPE=Release \
		-DENABLE_SHARED=OFF \
		-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_OSVER \
		-DCMAKE_OSX_ARCHITECTURES=$ARCH
	cmake --build . -j
	if [ $ARCH = "x86_64" ]; then
		make DESTDIR=$PWD/../../libjpeg install
		mv ../../libjpeg/opt/libjpeg-turbo/* ../../libjpeg
		rm -rf ../../libjpeg/opt
		mv ../../libjpeg/lib/libjpeg.a ../../libjpeg/lib/templib_$ARCH.a
	else
		mv libjpeg.a ../../libjpeg/lib/templib_$ARCH.a
	fi
	cd ..; rm -rf build
done

# repack into one .a
cd ../libjpeg/lib
lipo -create templib_*.a -output libjpeg.a
rm templib_*.a
rm libturbojpeg.a

echo "libjpeg build successful"

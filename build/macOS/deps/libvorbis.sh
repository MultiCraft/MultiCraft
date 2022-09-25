#!/bin/bash -e

. sdk.sh
VORBIS_VERSION=1.3.7

if [ ! -d vorbis-src ]; then
	wget https://github.com/xiph/vorbis/releases/download/v$VORBIS_VERSION/libvorbis-$VORBIS_VERSION.tar.gz
	tar -xzvf libvorbis-$VORBIS_VERSION.tar.gz
	mv libvorbis-$VORBIS_VERSION libvorbis-src
	rm libvorbis-$VORBIS_VERSION.tar.gz
	mkdir libvorbis-src/build
fi

rm -rf libvorbis

cd libvorbis-src/build

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DOGG_LIBRARY="../../libogg/libogg.a" -DOGG_INCLUDE_DIR="../../libogg/include" \
	-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" -DCMAKE_CXX_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_OSVER \
	-DCMAKE_OSX_ARCHITECTURES=$OSX_ARCHITECTURES
cmake --build . -j

mkdir -p ../../libvorbis
cp -r lib/libvorbis.a ../../libvorbis/libvorbis.a
cp -r lib/libvorbisfile.a ../../libvorbis/libvorbisfile.a
cp -r ../include ../../libvorbis/include

echo "Vorbis build successful"

#!/bin/bash -e

VORBIS_VERSION=1.3.7

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d libvorbis-src ]; then
	git clone -b v$VORBIS_VERSION --depth 1 https://github.com/xiph/vorbis libvorbis-src
	mkdir libvorbis-src/build
fi

rm -rf libvorbis

cd libvorbis-src/build

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DOGG_LIBRARY="../../libogg/libogg.a" \
	-DOGG_INCLUDE_DIR="../../libogg/include" \
	-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_CXX_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_OSX_ARCHITECTURES=$OSX_ARCHITECTURES

cmake --build . -j

mkdir -p ../../libvorbis
cp -v lib/libvorbis.a ../../libvorbis
cp -v lib/libvorbisfile.a ../../libvorbis
cp -rv ../include ../../libvorbis/include

echo "Vorbis build successful"

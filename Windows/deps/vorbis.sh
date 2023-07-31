#!/bin/bash -e

. ./sdk.sh
VORBIS_VERSION=1.3.7

if [ ! -d libvorbis-src ]; then
	git clone -b v$VORBIS_VERSION --depth 1 https://github.com/xiph/vorbis libvorbis-src
	mkdir libvorbis-src/build
fi

cd libvorbis-src/build

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DOGG_LIBRARY="../../libogg/libogg.a" \
	-DOGG_INCLUDE_DIR="../../libogg/include" \
	-DCMAKE_C_FLAGS_RELEASE="$CFLAGS" \
	-DCMAKE_CXX_FLAGS="$CXXFLAGS -fPIC"

cmake --build . -j${NPROC}

# update `include` folder
rm -rf ../../libvorbis/include
mkdir -p ../../libvorbis/include
cp -a ../include ../../libvorbis
# update lib
rm -rf ../../libvorbis/lib
mkdir -p ../../libvorbis/lib
cp -a lib/*.a ../../libvorbis/lib

echo "Vorbis build successful"

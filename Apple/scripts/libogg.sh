#!/bin/bash -e

OGG_VERSION=1.3.5

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d libogg-src ]; then
	git clone -b v$OGG_VERSION --depth 1 https://github.com/xiph/ogg libogg-src
	mkdir libogg-src/build
fi

rm -rf libogg

cd libogg-src/build

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_CXX_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_OSX_ARCHITECTURES=$OSX_ARCHITECTURES

cmake --build . -j

mkdir -p ../../libogg
cp -v libogg.a ../../libogg
cp -rv ../include ../../libogg/include

echo "Ogg build successful"

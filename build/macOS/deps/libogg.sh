#!/bin/bash -e

. sdk.sh
OGG_VERSION=1.3.5

if [ ! -d ogg-src ]; then
	wget https://github.com/xiph/ogg/releases/download/v$OGG_VERSION/libogg-$OGG_VERSION.tar.gz
	tar -xzvf libogg-$OGG_VERSION.tar.gz
	mv libogg-$OGG_VERSION libogg-src
	rm libogg-$OGG_VERSION.tar.gz
	mkdir libogg-src/build
fi

cd libogg-src/build

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" -DCMAKE_CXX_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_OSVER \
	-DCMAKE_OSX_ARCHITECTURES=$OSX_ARCHITECTURES
cmake --build . -j

mkdir -p ../../libogg
cp -r libogg.a ../../libogg/libogg.a
cp -r ../include ../../libogg/include

echo "Ogg build successful"

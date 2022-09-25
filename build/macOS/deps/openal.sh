#!/bin/bash -e

. sdk.sh
OPENAL_VERSION=1.22.2

if [ ! -d openal-src ]; then
	wget https://github.com/kcat/openal-soft/archive/$OPENAL_VERSION.tar.gz
	tar -xzvf $OPENAL_VERSION.tar.gz
	mv openal-soft-$OPENAL_VERSION openal-src
	rm $OPENAL_VERSION.tar.gz
fi

rm -rf openal

cd openal-src

cmake -S . \
	-DCMAKE_BUILD_TYPE=Release \
	-DLIBTYPE=STATIC \
	-DALSOFT_REQUIRE_COREAUDIO=ON \
	-DALSOFT_EMBED_HRTF_DATA=ON -DALSOFT_UTILS=OFF \
	-DALSOFT_EXAMPLES=OFF -DALSOFT_INSTALL=OFF -DALSOFT_BACKEND_WAVE=OFF \
	-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" -DCMAKE_CXX_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_OSVER \
	-DCMAKE_OSX_ARCHITECTURES=$OSX_ARCHITECTURES
cmake --build . -j

mkdir -p ../openal
cp -r libopenal.a ../openal/libopenal.a
cp -r include ../openal/include

echo "OpenAL-Soft build successful"

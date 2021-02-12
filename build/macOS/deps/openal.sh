#!/bin/bash -e

. sdk.sh
OPENAL_VERSION=1.21.1

if [ ! -d openal-src ]; then
	wget https://github.com/kcat/openal-soft/archive/$OPENAL_VERSION.tar.gz
	tar -xzvf $OPENAL_VERSION.tar.gz
	mv openal-soft-$OPENAL_VERSION openal-src
	rm $OPENAL_VERSION.tar.gz
fi

cd openal-src

cmake -S . \
	-DCMAKE_CXX_EXTENSIONS=OFF -DALSOFT_REQUIRE_COREAUDIO=ON \
	-DALSOFT_EMBED_HRTF_DATA=YES -DALSOFT_UTILS=OFF \
	-DALSOFT_EXAMPLES=OFF -DALSOFT_INSTALL=OFF -DALSOFT_BACKEND_WAVE=NO \
	-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS" -DCMAKE_CXX_FLAGS_RELEASE="$OSX_FLAGS" \
	-DCMAKE_OSX_DEPLOYMENT_TARGET=10.10 \
	"-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64"
cmake --build .

mkdir -p ../openal
mv libopenal.$OPENAL_VERSION.dylib ../openal/libopenal.1.dylib

echo "OpenAL-Soft build successful"

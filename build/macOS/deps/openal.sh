#!/bin/bash -e

. sdk.sh
OPENAL_VERSION=1.22.2

if [ ! -d openal-src ]; then
	git clone -b $OPENAL_VERSION --depth 1 https://github.com/kcat/openal-soft openal-src
fi

rm -rf openal

cd openal-src

cmake -S . \
	-DCMAKE_BUILD_TYPE=Release \
	-DLIBTYPE=STATIC \
	-DALSOFT_REQUIRE_COREAUDIO=ON \
	-DALSOFT_EMBED_HRTF_DATA=ON \
	-DALSOFT_UTILS=OFF \
	-DALSOFT_EXAMPLES=OFF \
	-DALSOFT_BACKEND_WAVE=OFF \
	-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_CXX_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_OSX_ARCHITECTURES=$OSX_ARCHITECTURES

cmake --build . -j

mkdir -p ../openal
cp -v libopenal.a ../openal
cp -rv include ../openal/include

echo "OpenAL-Soft build successful"

#!/bin/bash -e

. sdk.sh
OPENAL_VERSION=1.22.2

if [ ! -d openal-src ]; then
	wget https://github.com/kcat/openal-soft/archive/$OPENAL_VERSION.tar.gz
	tar -xzvf $OPENAL_VERSION.tar.gz
	mv openal-soft-$OPENAL_VERSION openal-src
	rm $OPENAL_VERSION.tar.gz
fi

cd openal-src

cmake -S . \
	-DALSOFT_REQUIRE_COREAUDIO=ON \
	-DALSOFT_EMBED_HRTF_DATA=YES -DALSOFT_UTILS=OFF \
	-DALSOFT_EXAMPLES=OFF -DALSOFT_INSTALL=OFF -DALSOFT_BACKEND_WAVE=OFF \
	-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" -DCMAKE_CXX_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_OSVER \
	-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build . -j

# Hack: because we strip and sign the library when linking
codesign --remove-signature libopenal.$OPENAL_VERSION.dylib

mkdir -p ../openal
mv libopenal.$OPENAL_VERSION.dylib ../openal/libopenal.1.dylib

echo "OpenAL-Soft build successful"

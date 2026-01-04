#!/bin/bash -e

OPENAL_VERSION=1.25.0

. scripts/sdk.sh
mkdir -p deps; cd deps

[ ! -d openal-src ] && \
	git clone -b $OPENAL_VERSION --depth 1 https://github.com/kcat/openal-soft openal-src

rm -rf openal

cd openal-src

for ARCH in x86_64 arm64
do
	echo "Building OpenAL-Soft for $ARCH"
	rm -rf build_$ARCH
	cmake -S . -B build_$ARCH \
		-DCMAKE_BUILD_TYPE=Release \
		-DLIBTYPE=STATIC \
		-DALSOFT_REQUIRE_COREAUDIO=ON \
		-DALSOFT_EMBED_HRTF_DATA=ON \
		-DALSOFT_UTILS=OFF \
		-DALSOFT_EXAMPLES=OFF \
		-DALSOFT_BACKEND_WAVE=OFF \
		-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_CXX_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_OSX_ARCHITECTURES=$ARCH
	cmake --build build_$ARCH -j

	cp build_$ARCH/libopenal.a templib_$ARCH.a
done

# repack into one .a
lipo -create templib_*.a -output libopenal.a
rm templib_*.a

mkdir -p ../openal
cp -v libopenal.a ../openal
cp -rv include ../openal/include

echo "OpenAL-Soft build successful"

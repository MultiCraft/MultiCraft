#!/bin/bash -e

SDL_VERSION=release-3.4.0

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d libSDL-src ]; then
	git clone -b $SDL_VERSION --depth 1 https://github.com/libsdl-org/SDL.git libSDL-src
	sed -i '' 's/^#pragma STDC FENV_ACCESS ON/\/\/&/' libSDL-src/src/audio/SDL_audiotypecvt.c
fi

rm -rf libSDL

mkdir -p libSDL/include

cd libSDL-src

for ARCH in x86_64 arm64
do
	echo "Building libSDL for $ARCH"
	mkdir -p build; cd build
	cmake .. \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_OBJC_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_INSTALL_PREFIX="." \
		-DCMAKE_OSX_ARCHITECTURES=$ARCH \
		-DBUILD_SHARED_LIBS=OFF \
		-DSDL_AUDIO=OFF \
		-DSDL_RENDER=OFF \
		-DSDL_CAMERA=OFF \
		-DSDL_METAL=OFF \
		-DSDL_GPU=OFF \
		-DSDL_HAPTIC=OFF \
		-DSDL_POWER=OFF \
		-DSDL_DIALOG=OFF \
		-DSDL_TESTS=OFF \
		-DSDL_VULKAN=OFF \
		-DSDL_EXAMPLES=OFF

	cmake --build . -j

	if [ $ARCH = "x86_64" ]; then
		cp -rv ../include ../../libSDL
	fi
	cp -v libSDL3.a ../../libSDL/templib_$ARCH.a

	cd ..; rm -rf build
done

# repack into one .a
cd ../libSDL
lipo -create templib_*.a -output libSDL.a
rm templib_*.a

echo "libSDL build successful"

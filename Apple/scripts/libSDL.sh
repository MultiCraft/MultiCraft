#!/bin/bash -e

SDL_VERSION=release-3.2.20

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d libSDL-src ]; then
	wget https://github.com/libsdl-org/SDL/archive/$SDL_VERSION.tar.gz
	tar -xzf $SDL_VERSION.tar.gz
	mv SDL-$SDL_VERSION libSDL-src
	rm $SDL_VERSION.tar.gz
	# Disable some features that are not needed
	sed -i '' 's/#define SDL_AUDIO_DRIVER_COREAUDIO  1/#define SDL_AUDIO_DRIVER_COREAUDIO  0/g' libSDL-src/include/build_config/SDL_build_config_macos.h
	sed -i '' 's/#define SDL_AUDIO_DRIVER_DISK   1/#define SDL_AUDIO_DRIVER_DISK   0/g' libSDL-src/include/build_config/SDL_build_config_macos.h
	sed -i '' 's/#define SDL_AUDIO_DRIVER_DUMMY  1/#define SDL_AUDIO_DRIVER_DUMMY  0/g' libSDL-src/include/build_config/SDL_build_config_macos.h
	sed -i '' 's/#define SDL_PLATFORM_SUPPORTS_METAL    1/#define SDL_PLATFORM_SUPPORTS_METAL    0/g' libSDL-src/include/build_config/SDL_build_config_macos.h
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
		-DCMAKE_ASM_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_OBJC_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_INSTALL_PREFIX="." \
		-DCMAKE_OSX_ARCHITECTURES=$ARCH \
		-DBUILD_SHARED_LIBS=OFF

	cmake --build . -j

	if [ $ARCH = "x86_64" ]; then
		cp -rv ../include ../../libSDL
		cp -v libSDL3.a ../../libSDL/templib_$ARCH.a
	else
		cp -v libSDL3.a ../../libSDL/templib_$ARCH.a
	fi

	cd ..; rm -rf build
done

# repack into one .a
cd ../libSDL
lipo -create templib_*.a -output libSDL.a
rm templib_*.a

echo "libSDL build successful"

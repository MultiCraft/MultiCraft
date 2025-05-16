#!/bin/bash -e

SDL3_VERSION=release-3.2.8

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d SDL3-src ]; then
	wget https://github.com/libsdl-org/SDL/archive/$SDL3_VERSION.tar.gz
	tar -xzf $SDL3_VERSION.tar.gz
	mv SDL-$SDL3_VERSION SDL3-src
	rm $SDL3_VERSION.tar.gz
	# Disable some features that are not needed
	sed -i '' 's/#define SDL_AUDIO_DRIVER_COREAUDIO  1/#define SDL_AUDIO_DRIVER_COREAUDIO  0/g' SDL3-src/include/build_config/SDL_build_config_macos.h
	sed -i '' 's/#define SDL_AUDIO_DRIVER_DISK   1/#define SDL_AUDIO_DRIVER_DISK   0/g' SDL3-src/include/build_config/SDL_build_config_macos.h
	sed -i '' 's/#define SDL_AUDIO_DRIVER_DUMMY  1/#define SDL_AUDIO_DRIVER_DUMMY  0/g' SDL3-src/include/build_config/SDL_build_config_macos.h
	sed -i '' 's/#define SDL_PLATFORM_SUPPORTS_METAL    1/#define SDL_PLATFORM_SUPPORTS_METAL    0/g' SDL3-src/include/build_config/SDL_build_config_macos.h
fi

rm -rf SDL3

mkdir -p SDL3/include

cd SDL3-src

for ARCH in x86_64 arm64
do
	echo "Building SDL3 for $ARCH"
	mkdir -p build; cd build
	cmake .. \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_ASM_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_INSTALL_PREFIX="." \
		-DCMAKE_OSX_ARCHITECTURES=$ARCH \
		-DBUILD_SHARED_LIBS=OFF

	cmake --build . -j

	if [ $ARCH = "x86_64" ]; then
		cp -rv ../include ../../SDL3
		cp -v libSDL3.a ../../SDL3/templib_$ARCH.a
	else
		cp -v libSDL3.a ../../SDL3/templib_$ARCH.a
	fi

	cd ..; rm -rf build
done

# repack into one .a
cd ../SDL3
lipo -create templib_*.a -output libSDL3.a
rm templib_*.a

echo "SDL3 build successful"

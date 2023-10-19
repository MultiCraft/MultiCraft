#!/bin/bash -e

SDL2_VERSION=release-2.28.4

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d SDL2-src ]; then
	wget https://github.com/libsdl-org/SDL/archive/$SDL2_VERSION.tar.gz
	tar -xzf $SDL2_VERSION.tar.gz
	mv SDL-$SDL2_VERSION SDL2-src
	rm $SDL2_VERSION.tar.gz
	# Disable some features that are not needed
	sed -i '' 's/#define SDL_AUDIO_DRIVER_COREAUDIO  1/#define SDL_AUDIO_DRIVER_COREAUDIO  0/g' SDL2-src/include/SDL_config_macosx.h
	sed -i '' 's/#define SDL_AUDIO_DRIVER_DISK   1/#define SDL_AUDIO_DRIVER_DISK   0/g' SDL2-src/include/SDL_config_macosx.h
	sed -i '' 's/#define SDL_AUDIO_DRIVER_DUMMY  1/#define SDL_AUDIO_DRIVER_DUMMY  0/g' SDL2-src/include/SDL_config_macosx.h
	sed -i '' 's/#define SDL_PLATFORM_SUPPORTS_METAL    1/#define SDL_PLATFORM_SUPPORTS_METAL    0/g' SDL2-src/include/SDL_config_macosx.h
fi

rm -rf SDL2

cd SDL2-src

xcodebuild build \
	 ARCHS="$OSX_ARCHES" \
	-project Xcode/SDL/SDL.xcodeproj \
	-configuration Release \
	-scheme "Static Library"

BUILD_FOLDER=$(xcodebuild \
		-project Xcode/SDL/SDL.xcodeproj \
		-configuration Release \
		-scheme "Static Library" \
		-showBuildSettings | \
		grep TARGET_BUILD_DIR | sed -n -e 's/^.*TARGET_BUILD_DIR = //p')

mkdir -p ../SDL2
cp -v "${BUILD_FOLDER}/libSDL2.a" ../SDL2
cp -rv include ../SDL2

echo "SDL2 build successful"

#!/bin/bash -e

. sdk.sh
SDL2_VERSION=2.0.22

if [ ! -d SDL2-src ]; then
	wget https://github.com/libsdl-org/SDL/archive/release-$SDL2_VERSION.tar.gz
	tar -xzvf release-$SDL2_VERSION.tar.gz
	mv SDL-release-$SDL2_VERSION SDL2-src
	rm release-$SDL2_VERSION.tar.gz
	# patch SDL2
	patch -p1 < SDL2.diff
fi

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
cp -a "${BUILD_FOLDER}/libSDL2.a" ../SDL2
cp -a include ../SDL2

echo "SDL2 build successful"

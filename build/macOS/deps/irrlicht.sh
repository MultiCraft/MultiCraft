#!/bin/bash -e

. sdk.sh

[ ! -d irrlicht-src ] && \
	git clone --depth 1 -b SDL2 https://github.com/deveee/Irrlicht irrlicht-src

SDL2_PATH="$PWD/SDL2/SDL2.framework/Headers"

cd irrlicht-src/source/Irrlicht
xcodebuild build \
	 ARCHS="$OSX_ARCHES" \
	 OTHER_CFLAGS="-D_IRR_COMPILE_WITH_SDL2_DEVICE_ -I$SDL2_PATH" \
	-project Irrlicht.xcodeproj \
	-configuration Release \
	-scheme Irrlicht_OSX

BUILD_FOLDER=$(xcodebuild -project Irrlicht.xcodeproj -scheme \
		Irrlicht_OSX -showBuildSettings | \
		grep TARGET_BUILD_DIR | sed -n -e 's/^.*TARGET_BUILD_DIR = //p')

cd ../..

[ -d ../irrlicht ] && rm -r ../irrlicht
mkdir -p ../irrlicht
cp "${BUILD_FOLDER}/libIrrlicht.a" ../irrlicht
cp -r include ../irrlicht/include

echo "Irrlicht build successful"

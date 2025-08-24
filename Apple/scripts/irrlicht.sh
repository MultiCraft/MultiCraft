#!/bin/bash -e

. scripts/sdk.sh
mkdir -p deps; cd deps

[ ! -d irrlicht-src ] && \
	git clone --depth 1 -b SDL3 https://github.com/deveee/Irrlicht irrlicht-src

rm -rf irrlicht

cd irrlicht-src/source/Irrlicht

xcodebuild build \
	 ARCHS="$OSX_ARCHES" \
	 OTHER_CFLAGS="-I../../../libpng/include -I../../../libjpeg/include" \
	-project Irrlicht.xcodeproj \
	-configuration Release \
	-scheme Irrlicht_OSX

BUILD_FOLDER=$(xcodebuild -project Irrlicht.xcodeproj -scheme \
		Irrlicht_OSX -showBuildSettings | \
		grep TARGET_BUILD_DIR | sed -n -e 's/^.*TARGET_BUILD_DIR = //p')

cd ../..

[ -d ../irrlicht ] && rm -r ../irrlicht
mkdir -p ../irrlicht
cp -v "${BUILD_FOLDER}/libIrrlicht.a" ../irrlicht
cp -rv include ../irrlicht/include

echo "Irrlicht build successful"

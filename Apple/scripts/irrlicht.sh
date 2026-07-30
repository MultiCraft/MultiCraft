#!/bin/bash -e

. scripts/sdk.sh
mkdir -p deps; cd deps

[ ! -d irrlicht-src ] && \
	git clone --depth 1 -b SDL https://github.com/MoNTE48/Irrlicht irrlicht-src

DEPS_DIR=$PWD

rm -rf irrlicht

cd irrlicht-src/source/Irrlicht

xcodebuild build \
	 ARCHS="$OSX_ARCHES" \
	 OTHER_CFLAGS="-I$DEPS_DIR/libpng/include -I$DEPS_DIR/libjpeg/include -I$DEPS_DIR/libSDL/include -I$DEPS_DIR/angle/include" \
	-project Irrlicht.xcodeproj \
	-configuration Release \
	-scheme Irrlicht_OSX

BUILD_FOLDER=$(xcodebuild -project Irrlicht.xcodeproj -scheme \
		Irrlicht_OSX -showBuildSettings | \
		grep TARGET_BUILD_DIR | sed -n -e 's/^.*TARGET_BUILD_DIR = //p')

cd ../..

[ -d "$DEPS_DIR/irrlicht" ] && rm -r "$DEPS_DIR/irrlicht"
mkdir -p "$DEPS_DIR/irrlicht"
cp -v "${BUILD_FOLDER}/libIrrlicht.a" "$DEPS_DIR/irrlicht"
cp -rv include "$DEPS_DIR/irrlicht/include"
cp -r media/Shaders "$DEPS_DIR/irrlicht/shaders"

echo "Irrlicht build successful"

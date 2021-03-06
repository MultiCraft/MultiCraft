#!/bin/bash -e

. sdk.sh

[ ! -d irrlicht-src ] && \
	git clone --depth 1 -b ogl-es https://github.com/MoNTE48/Irrlicht irrlicht-src

cd irrlicht-src/source/Irrlicht
xcodebuild build \
	 ARCHS="$OSX_ARCHES" \
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

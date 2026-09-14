#!/bin/bash -e

. scripts/sdk.sh
mkdir -p deps; cd deps

[ ! -d irrlicht-src ] && \
git clone --depth 1 -b SDL https://github.com/MoNTE48/Irrlicht irrlicht-src

# The C++ interface to Metal, header only and pinned to the SDK in use
METAL_CPP_VERSION=release/metal-cpp_macOS26.4_iOS26.4
if [ ! -d metal-cpp ]; then
	git clone -q --depth 1 -b $METAL_CPP_VERSION https://github.com/apple/metal-cpp metal-cpp
fi

DEPS_DIR=$PWD

rm -rf irrlicht

cd irrlicht-src/source/Irrlicht

xcodebuild build \
	 ARCHS="$OSX_ARCHES" \
	 OTHER_CFLAGS="\$(inherited) -I$DEPS_DIR/libpng/include -I$DEPS_DIR/libSDL/include -I$DEPS_DIR/metal-cpp" \
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
# The folder the app copies to client/shaders
mkdir -p "$DEPS_DIR/irrlicht/Irrlicht"

# The driver's MSL stages from its common GLSL ones, in buffer slots 28 and 29 CMetalDriver binds
metal_stage() {
	glslangValidator -V --shift-ubo-binding $3 $4 -o "$DEPS_DIR/$2.spv" media/Shaders/Common/$1
	spirv-cross --msl --msl-version 20100 --msl-decoration-binding --rename-entry-point main $2 $3 \
		"$DEPS_DIR/$2.spv" --output "$DEPS_DIR/irrlicht/Irrlicht/$2.metal"
	xcrun -sdk macosx metal -mmacosx-version-min=$OSX_OSVER -c "$DEPS_DIR/irrlicht/Irrlicht/$2.metal" \
		-o "$DEPS_DIR/$2.air"
	rm "$DEPS_DIR/$2.spv" "$DEPS_DIR/$2.air"
}
metal_stage CDriverCommon.vert vs_fixed3d vert 28
metal_stage CDriverCommon.frag fs_fixed3d frag 29
metal_stage CDriverCommonDepth.vert vs_depth_ndc vert 28

echo "Irrlicht build successful"

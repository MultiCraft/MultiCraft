#!/bin/bash -e

. sdk.sh

mkdir -p output/irrlicht/lib/$TARGET_ABI
mkdir -p deps; cd deps

[ ! -d irrlicht-src ] && \
	git clone --depth 1 -b SDL2 https://github.com/MoNTE48/Irrlicht irrlicht-src

cd irrlicht-src/source/Irrlicht/Android-SDL2

export SDL2_PATH="$OUTPUT_PATH/sdl2/"
$ANDROID_NDK/ndk-build -j \
	NDEBUG=1 \
	APP_ABI="$TARGET_ABI" \
	APP_PLATFORM=android-"$API" \
	APP_CFLAGS="$CFLAGS" \
	APP_CXXFLAGS="$CXXFLAGS -std=gnu++17" \
	APP_CPPFLAGS="$APP_CXXFLAGS -DNO_IRR_COMPILE_WITH_SDL_TEXTINPUT_ -I$OUTPUT_PATH/libjpeg/include -I$OUTPUT_PATH/libpng/include" \
	APP_STL="c++_static"

# update `include` folder
rm -rf ../../../../../output/irrlicht/include
cp -r ../../../include ../../../../../output/irrlicht/include
# update lib
rm -rf ../../../../../../../Irrlicht/lib/$TARGET_ABI/libIrrlicht.a
cp -r ../../../lib/Android-SDL2/$TARGET_ABI/libIrrlicht.a ../../../../../output/irrlicht/lib/$TARGET_ABI/libIrrlicht.a
# update shaders
rm -rf ../../../../../output/irrlicht/shaders
cp -r ../../../media/Shaders ../../../../../output/irrlicht/shaders

echo "Irrlicht build successful"

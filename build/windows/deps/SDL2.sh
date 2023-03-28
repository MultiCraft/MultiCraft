#!/bin/bash -e

SDL2_VERSION=2.26.4

. sdk.sh

mkdir -p output/sdl2/lib/$TARGET_ABI
mkdir -p deps; cd deps

if [ ! -d SDL2-src ]; then
	wget https://github.com/libsdl-org/SDL/archive/release-$SDL2_VERSION.tar.gz
	tar -xzvf release-$SDL2_VERSION.tar.gz
	mv SDL-release-$SDL2_VERSION sdl2-src
	rm release-$SDL2_VERSION.tar.gz
fi

cd SDL2-src

mkdir -p build; cd build

cmake .. -DANDROID_STL="c++_static" \
    -DANDROID_NATIVE_API_LEVEL="$NATIVE_API_LEVEL" \
    -DANDROID_ABI="$ANDROID_ABI" \
    -DANDROID_PLATFORM="$API" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="$CFLAGS" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS -fPIC" \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
    -DLIBTYPE=STATIC

cmake --build . -j

# update `include` folder
rm -rf ../../../output/sdl2/include/
cp -r ../include ../../../output/sdl2/include
# update lib
rm -rf ../../../output/sdl2/lib/$TARGET_ABI/libSDL2.a
cp -r libSDL2.a ../../../output/sdl2/lib/$TARGET_ABI/libSDL2.a

echo "SDL2 build successful"

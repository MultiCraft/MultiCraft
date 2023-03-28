#!/bin/bash -e

. sdk.sh
PNG_VERSION=1.6.39

mkdir -p output/libpng/lib/$TARGET_ABI
mkdir -p deps; cd deps

if [ ! -d libpng-src ]; then
	wget https://download.sourceforge.net/libpng/libpng-$PNG_VERSION.tar.gz
	tar -xzvf libpng-$PNG_VERSION.tar.gz
	mv libpng-$PNG_VERSION libpng-src
	rm libpng-$PNG_VERSION.tar.gz
	mkdir libpng-src/build
fi

cd libpng-src/build

cmake .. -DANDROID_STL="c++_static" -DANDROID_NATIVE_API_LEVEL="$NATIVE_API_LEVEL" \
	-DCMAKE_BUILD_TYPE=Release \
	-DPNG_SHARED=OFF \
	-DPNG_TESTS=OFF \
	-DPNG_EXECUTABLES=OFF \
	-DANDROID_ABI="$ANDROID_ABI" \
	-DANDROID_PLATFORM="$API" \
	-DCMAKE_C_FLAGS_RELEASE="$CFLAGS" \
	-DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake"

cmake --build . -j

# update `include` folder
rm -rf ../../../output/libpng/include
mkdir -p ../../../output/libpng/include
cp -v ../*.h ../../../output/libpng/include
cp -v pnglibconf.h ../../../output/libpng/include
# update lib
rm -rf ../../../output/libpng/lib/$TARGET_ABI/libpng.a
cp -r libpng16.a ../../../output/libpng/lib/$TARGET_ABI/libpng.a

echo "libpng build successful"

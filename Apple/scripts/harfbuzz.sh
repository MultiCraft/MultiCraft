#!/bin/bash -e

HARFBUZZ_VERSION=12.3.2

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d harfbuzz-src ]; then
	git clone -b $HARFBUZZ_VERSION --depth 1 https://github.com/harfbuzz/harfbuzz harfbuzz-src
	mkdir harfbuzz-src/build
fi

rm -rf harfbuzz

FREETYPE_INCLUDE="$(pwd)/freetype/include"

cd harfbuzz-src/build

cmake .. \
	-DBUILD_SHARED_LIBS=FALSE \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_CXX_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_OSX_ARCHITECTURES=$OSX_ARCHITECTURES \
	-DFREETYPE_LIBRARY="../../freetype/lib/$TARGET_ABI/libfreetype.a ../../libpng/lib/$TARGET_ABI/libpng.a" \
	-DFREETYPE_INCLUDE_DIRS=$FREETYPE_INCLUDE \
	-DHB_HAVE_GLIB=OFF \
	-DHB_HAVE_GOBJECT=OFF \
	-DHB_HAVE_ICU=OFF \
	-DHB_HAVE_FREETYPE=ON \
	-DHB_BUILD_SUBSET=OFF

cmake --build . -j

mkdir -p ../../harfbuzz/include/harfbuzz
cp -v libharfbuzz.a ../../harfbuzz
cp -v ../src/*.h ../../harfbuzz/include/harfbuzz

echo "Freetype build successful"

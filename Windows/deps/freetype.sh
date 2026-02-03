#!/bin/bash -e

FREETYPE_VERSION=2.14.1

. ./sdk.sh

export DEPS_ROOT=$(pwd)

if [ ! -d freetype-src ]; then
	wget https://sourceforge.net/projects/freetype/files/freetype2/$FREETYPE_VERSION/freetype-$FREETYPE_VERSION.tar.xz
	tar -xaf freetype-$FREETYPE_VERSION.tar.xz
	mv freetype-$FREETYPE_VERSION freetype-src
	rm freetype-$FREETYPE_VERSION.tar.xz
	mkdir freetype-src/build-bootstrap
	mkdir freetype-src/build
fi

if [ ! -z "$1" ] && [ "$1" = "bootstrap" ]; then
	cd freetype-src/build-bootstrap
	HARFBUZZ_FLAGS="-DFT_DISABLE_HARFBUZZ=TRUE"
else
	cd freetype-src/build
	HARFBUZZ_FLAGS=" \
		-DFT_REQUIRE_HARFBUZZ=TRUE \
		-DFT_DYNAMIC_HARFBUZZ=FALSE \
		-DHarfBuzz_LIBRARY=$DEPS_ROOT/harfbuzz/lib/libharfbuzz.a \
		-DHarfBuzz_INCLUDE_DIRS=$DEPS_ROOT/harfbuzz/include"
fi

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_SHARED_LIBS=FALSE \
	-DCMAKE_C_FLAGS_RELEASE="$CFLAGS" \
	-DFT_DISABLE_BZIP2=TRUE \
	-DFT_DISABLE_BROTLI=TRUE \
	-DFT_DISABLE_PNG=TRUE \
	-DFT_REQUIRE_ZLIB=TRUE \
	$HARFBUZZ_FLAGS \
	-DZLIB_LIBRARY="$DEPS_ROOT/zlib/lib/libz.a" \
	-DZLIB_INCLUDE_DIRS="$DEPS_ROOT/zlib/include"

cmake --build . -j${NPROC}

# update `include` folder
rm -rf ../../freetype/include
mkdir -p ../../freetype/include
cp -a ../include ../../freetype
cp -a ./include ../../freetype
rm -rf ../../freetype/include/dlg
# update lib
rm -rf ../../freetype/lib
mkdir -p ../../freetype/lib
cp libfreetype.a ../../freetype/lib

echo "Freetype build successful"

#!/bin/bash -e

FREETYPE_VERSION=2.14.1

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d freetype-src ]; then
	wget http://download.savannah.gnu.org/releases/freetype/freetype-$FREETYPE_VERSION.tar.gz
	tar -xzf freetype-$FREETYPE_VERSION.tar.gz
	mv freetype-$FREETYPE_VERSION freetype-src
	rm freetype-$FREETYPE_VERSION.tar.gz
	mkdir freetype-src/build-bootstrap
	mkdir freetype-src/build
	sed -i '' 's/__attribute__(( visibility( "default" ) ))//g' freetype-src/include/freetype/config/public-macros.h
fi

rm -rf freetype

if [ ! -z "$1" ] && [ "$1" = "bootstrap" ]; then
	cd freetype-src/build-bootstrap
	HARFBUZZ_FLAGS="-DFT_DISABLE_HARFBUZZ=TRUE"
else
	cd freetype-src/build
	HARFBUZZ_FLAGS=" \
		-DFT_REQUIRE_HARFBUZZ=TRUE \
		-DFT_DYNAMIC_HARFBUZZ=FALSE \
		-DHarfBuzz_LIBRARY=../../harfbuzz/lib/$TARGET_ABI/libharfbuzz.a \
		-DHarfBuzz_INCLUDE_DIR=../../harfbuzz/include"
fi

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_SHARED_LIBS=FALSE \
	-DFT_DISABLE_BZIP2=TRUE \
	-DFT_DISABLE_BROTLI=TRUE \
	-DFT_REQUIRE_PNG=TRUE \
	-DFT_REQUIRE_ZLIB=TRUE \
	$HARFBUZZ_FLAGS \
	-DPNG_LIBRARY="../../libpng/libpng.a" \
	-DPNG_PNG_INCLUDE_DIR="../../libpng/include" \
	-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_OSX_ARCHITECTURES=$OSX_ARCHITECTURES

cmake --build . -j

mkdir -p ../../freetype
cp -v libfreetype.a ../../freetype
cp -rv ../include ../../freetype/include
rm -rf ../../freetype/include/dlg


echo "FreeType build successful"

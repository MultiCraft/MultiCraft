#!/bin/bash -e

FREETYPE_VERSION=2.13.2

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d freetype-src ]; then
	wget http://download.savannah.gnu.org/releases/freetype/freetype-$FREETYPE_VERSION.tar.gz
	tar -xzf freetype-$FREETYPE_VERSION.tar.gz
	mv freetype-$FREETYPE_VERSION freetype-src
	rm freetype-$FREETYPE_VERSION.tar.gz
	mkdir freetype-src/build
fi

rm -rf freetype

cd freetype-src/build

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_SHARED_LIBS=FALSE \
	-DFT_DISABLE_BZIP2=TRUE \
	-DFT_DISABLE_HARFBUZZ=TRUE \
	-DFT_DISABLE_BROTLI=TRUE \
	-DFT_REQUIRE_PNG=TRUE \
	-DFT_REQUIRE_ZLIB=TRUE \
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

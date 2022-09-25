#!/bin/bash -e

. sdk.sh
FREETYPE_VERSION=2.12.1

if [ ! -d freetype-src ]; then
	wget http://download.savannah.gnu.org/releases/freetype/freetype-$FREETYPE_VERSION.tar.gz
	tar -xzvf freetype-$FREETYPE_VERSION.tar.gz
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
	-DFT_DISABLE_PNG=TRUE \
	-DFT_DISABLE_HARFBUZZ=TRUE \
	-DFT_DISABLE_BROTLI=TRUE \
	-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_OSVER \
	-DCMAKE_OSX_ARCHITECTURES=$OSX_ARCHITECTURES

cmake --build . -j

mkdir -p ../../freetype
cp -r ../include ../../freetype/include
rm -rf ../../freetype/include/dlg
cp -r libfreetype.a ../../freetype/libfreetype.a


echo "FreeType build successful"

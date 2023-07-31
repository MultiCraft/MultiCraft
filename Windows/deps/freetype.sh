#!/bin/bash -e

FREETYPE_VERSION=2.13.1

. ./sdk.sh

if [ ! -d freetype-src ]; then
	wget https://download-mirror.savannah.gnu.org/releases/freetype/freetype-$FREETYPE_VERSION.tar.gz
	tar -xzf freetype-$FREETYPE_VERSION.tar.gz
	mv freetype-$FREETYPE_VERSION freetype-src
	rm freetype-$FREETYPE_VERSION.tar.gz
	mkdir freetype-src/build
fi

cd freetype-src/build

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_SHARED_LIBS=FALSE \
	-DFT_DISABLE_BZIP2=TRUE \
	-DFT_DISABLE_PNG=TRUE \
	-DFT_DISABLE_HARFBUZZ=TRUE \
	-DFT_DISABLE_BROTLI=TRUE \
	-DCMAKE_C_FLAGS_RELEASE="$CFLAGS"

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

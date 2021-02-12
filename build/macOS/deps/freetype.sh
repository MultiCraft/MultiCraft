#!/bin/bash -e

. sdk.sh
FREETYPE_VERSION=2.10.4

if [ ! -d freetype-src ]; then
	wget http://download.savannah.gnu.org/releases/freetype/freetype-$FREETYPE_VERSION.tar.gz
	tar -xzvf freetype-$FREETYPE_VERSION.tar.gz
	mv freetype-$FREETYPE_VERSION freetype-src
	rm freetype-$FREETYPE_VERSION.tar.gz
fi

cd freetype-src

CFLAGS=$OSX_FLAGS \
PKG_CONFIG=/bin/false \
./configure --prefix=/ \
	--disable-shared --enable-static \
	--with-bzip2=no --with-png=no
make -j

mkdir -p ../freetype
make DESTDIR=$PWD/../freetype install

echo "FreeType build successful"

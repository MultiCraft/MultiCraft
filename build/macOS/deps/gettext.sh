#!/bin/bash -e

. sdk.sh
GETTEXT_VERSION=0.21.1

if [ ! -d gettext-src ]; then
	wget https://ftp.gnu.org/pub/gnu/gettext/gettext-$GETTEXT_VERSION.tar.gz
	tar -xzvf gettext-$GETTEXT_VERSION.tar.gz
	mv gettext-$GETTEXT_VERSION gettext-src
	rm gettext-$GETTEXT_VERSION.tar.gz
fi

rm -rf gettext

cd gettext-src/gettext-runtime

CFLAGS="$OSX_FLAGS $OSX_ARCH -Dlocale_charset=intl_locale_charset" \
PKG_CONFIG=/bin/false \
./configure --prefix=/ \
	--disable-shared --enable-static

make -j
make DESTDIR=$PWD/build install

mkdir -p ../../gettext/include
cp -v build/include/libintl.h ../../gettext/include
cp -v build/lib/libintl.a ../../gettext

echo "gettext build successful"

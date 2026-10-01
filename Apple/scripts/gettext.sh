#!/bin/bash -e

GETTEXT_VERSION=1.0

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d gettext-src ]; then
	wget https://ftp.gnu.org/pub/gnu/gettext/gettext-$GETTEXT_VERSION.tar.gz
	tar -xzf gettext-$GETTEXT_VERSION.tar.gz
	mv gettext-$GETTEXT_VERSION gettext-src
	rm gettext-$GETTEXT_VERSION.tar.gz
fi

rm -rf gettext

cd gettext-src/gettext-runtime

# Without iconv: the catalogs and the codeset the game binds are both UTF-8
CFLAGS="$OSX_FLAGS $OSX_ARCH -Dlocale_charset=intl_locale_charset" \
PKG_CONFIG=/bin/false \
am_cv_func_iconv=no \
./configure --prefix=/ \
	--disable-shared --enable-static

make clean
make -j
make DESTDIR=$PWD/build install

mkdir -p ../../gettext/include
cp -v build/include/libintl.h ../../gettext/include
cp -v build/lib/libintl.a ../../gettext

echo "gettext build successful"

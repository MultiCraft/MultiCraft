#!/bin/bash -e

. sdk.sh
GETTEXT_VERSION=0.21

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

mkdir -p ../../gettext
make DESTDIR=$PWD/../../gettext install

echo "gettext build successful"

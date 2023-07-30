#!/bin/bash -e

. ./sdk.sh
GETTEXT_VERSION=0.22

if [ ! -d gettext-src ]; then
	wget https://ftp.gnu.org/pub/gnu/gettext/gettext-$GETTEXT_VERSION.tar.gz
	tar -xzf gettext-$GETTEXT_VERSION.tar.gz
	mv gettext-$GETTEXT_VERSION gettext-src
	rm gettext-$GETTEXT_VERSION.tar.gz
fi

cd gettext-src/gettext-runtime

./configure CFLAGS="$CFLAGS" CPPFLAGS="$CXXFLAGS" \
	--disable-shared --enable-static --disable-libasprintf

make -j$NPROC

# update `include` folder
rm -rf ../../gettext/include
mkdir -p ../../gettext/include
cp intl/libintl.h ../../gettext/include
# update lib
rm -rf ../../gettext/lib
mkdir -p ../../gettext/lib
cp intl/.libs/libintl.a ../../gettext/lib

echo "Gettext build successful"

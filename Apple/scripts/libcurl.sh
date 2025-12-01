#!/bin/bash -e

CURL_VERSION=8.14.1

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d libcurl-src ]; then
	wget https://curl.haxx.se/download/curl-$CURL_VERSION.tar.gz
	tar -xzf curl-$CURL_VERSION.tar.gz
	mv curl-$CURL_VERSION libcurl-src
	rm curl-$CURL_VERSION.tar.gz
fi

rm -rf libcurl

cd libcurl-src

CFLAGS="$OSX_FLAGS $OSX_ARCH" \
./configure --host=arm-apple-darwin --prefix=/ --disable-shared --enable-static \
	--disable-debug --disable-verbose --disable-versioned-symbols \
	--with-secure-transport --disable-dependency-tracking \
	--disable-ares --disable-cookies --disable-manual \
	--disable-proxy --disable-unix-sockets --without-librtmp \
	--disable-ftp --disable-ldap --disable-ldaps --disable-rtsp \
	--disable-dict --disable-telnet --disable-tftp --disable-pop3 \
	--disable-imap --disable-smtp --disable-gopher --disable-sspi \
	--disable-libcurl-option --without-libidn2 --without-nghttp2 \
	--without-libpsl

make -j

mkdir -p ../libcurl/include
cp -v lib/.libs/libcurl.a ../libcurl
cp -rv include/curl ../libcurl/include/curl

echo "libcurl build successful"

#!/bin/bash -e

. ./sdk.sh
CURL_VERSION=8.0.1

export DEPS_ROOT=$(pwd)

if [ ! -d libcurl-src ]; then
	wget https://curl.haxx.se/download/curl-$CURL_VERSION.tar.gz
	tar -xzvf curl-$CURL_VERSION.tar.gz
	mv curl-$CURL_VERSION libcurl-src
	rm curl-$CURL_VERSION.tar.gz
fi

cd libcurl-src

./configure \
	--with-mbedtls="$DEPS_ROOT/mbedtls" \
	--disable-shared --enable-static \
	--disable-debug --disable-verbose --disable-versioned-symbols \
	--disable-dependency-tracking --disable-libcurl-option \
	--disable-ares --disable-cookies --disable-crypto-auth --disable-manual \
	--disable-proxy --disable-unix-sockets --without-librtmp \
	--disable-ftp --disable-ldap --disable-ldaps --disable-rtsp \
	--disable-dict --disable-telnet --disable-tftp --disable-pop3 \
	--disable-imap --disable-smtp --disable-gopher --disable-sspi

make -j

# update `include` folder
rm -rf ../libcurl/include
mkdir -p ../libcurl/include
cp -a ./include ../libcurl
# update lib
rm -rf ../libcurl/lib
mkdir -p ../libcurl/lib
cp lib/.libs/libcurl.a ../libcurl/lib

echo "libcurl build successful"

#!/bin/bash -e

OPENSSL_VERSION=3.4.2

. ./sdk.sh

if [ ! -d openssl-src ]; then
	git clone -b openssl-$OPENSSL_VERSION --depth 1 https://github.com/openssl/openssl.git openssl-src
fi

cd openssl-src

./Configure no-tests no-shared
make -j${NPROC}

# update `include` folder
rm -rf ../openssl/include
mkdir -p ../openssl/include
cp -a ./include/* ../openssl/include
# update lib
rm -rf ../openssl/lib
mkdir -p ../openssl/lib
cp libcrypto.a ../openssl/lib
cp libssl.a ../openssl/lib

echo "OpenSSL build successful"

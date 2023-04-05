#!/bin/bash -e

. ./sdk.sh
MBEDTLS_VERSION=3.3.0

if [ ! -d mbedtls-src ]; then
	wget https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v$MBEDTLS_VERSION.tar.gz
	tar -xzvf v$MBEDTLS_VERSION.tar.gz
	mv mbedtls-$MBEDTLS_VERSION mbedtls-src
	rm v$MBEDTLS_VERSION.tar.gz
	mkdir mbedtls-src/build
fi

cd mbedtls-src/build

cmake .. \
	-DBUILD_SHARED_LIBS=OFF \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS="$CFLAGS" \
	-DCMAKE_CXX_FLAGS="$CXXFLAGS -fPIC" \
	-DENABLE_TESTING=OFF \
	-DENABLE_PROGRAMS=OFF

cmake --build . -j$NPROC

# update `include` folder
rm -rf ../../mbedtls/include
mkdir -p ../../mbedtls/include
cp -a ../include/mbedtls ../../mbedtls/include
cp -a ../include/psa ../../mbedtls/include
# update lib
rm -rf ../../mbedtls/lib
mkdir -p ../../mbedtls/lib
cp -a library/*.a ../../mbedtls/lib

# update lib
#~ rm -rf ../../../output/mbedtls/lib/$TARGET_ABI/*.a
#~ cp -r library/*.a ../../../output/mbedtls/lib/$TARGET_ABI/

echo "MbedTLS build successful"

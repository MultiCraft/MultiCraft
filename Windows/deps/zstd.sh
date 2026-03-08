#!/bin/bash -e

ZSTD_VERSION=1.5.7

. ./sdk.sh

if [ ! -d zstd-src ]; then
	wget -O zstd-v$ZSTD_VERSION.tar.gz https://github.com/facebook/zstd/archive/refs/tags/v$ZSTD_VERSION.tar.gz
	tar -xzf zstd-v$ZSTD_VERSION.tar.gz
	mv zstd-$ZSTD_VERSION zstd-src
	rm zstd-v$ZSTD_VERSION.tar.gz
fi

mkdir -p zstd-src/build/cmake/builddir
cd zstd-src/build/cmake/builddir

cmake .. -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS_RELEASE="$CFLAGS" \
	-DZSTD_MULTITHREAD_SUPPORT=ON \
	-DZSTD_BUILD_TESTS=OFF \
	-DZSTD_BUILD_PROGRAMS=OFF \
	-DZSTD_BUILD_STATIC=ON \
	-DZSTD_BUILD_SHARED=OFF

cmake --build . -j

# update `include` folder
rm -rf ../../../../zstd/include/
mkdir -p ../../../../zstd/include
cp -r ../../../lib/*.h ../../../../zstd/include
# update lib
rm -rf ../../../../zstd/lib/libzstd.a
mkdir -p ../../../../zstd/lib
cp -r ./lib/libzstd.a ../../../../zstd/lib/libzstd.a

echo "Zstd build successful"

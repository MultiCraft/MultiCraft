#!/bin/bash -e

LEVELDB_VERSION=1.23

. ./sdk.sh

if [ ! -d leveldb-src ]; then
	git clone -b $LEVELDB_VERSION --depth 1 https://github.com/google/leveldb leveldb-src
	mkdir leveldb-src/build
fi

cd leveldb-src/build

cmake .. \
	-DBUILD_SHARED_LIBS=OFF \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS="$CFLAGS" \
	-DCMAKE_CXX_FLAGS="$CXXFLAGS -fPIC" \
	-DLEVELDB_BUILD_TESTS=OFF \
	-DLEVELDB_BUILD_BENCHMARKS=OFF \
	-DLEVELDB_INSTALL=OFF

cmake --build . -j

# update `include` folder
rm -rf ../../leveldb/include
mkdir -p ../../leveldb/include
cp -a ../include ../../leveldb
cp -a ./include ../../leveldb
# update lib
rm -rf ../../leveldb/lib
mkdir -p ../../leveldb/lib
cp libleveldb.a ../../leveldb/lib

echo "LevelDB build successful"

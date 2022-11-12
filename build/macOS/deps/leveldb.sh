#!/bin/bash -e

. sdk.sh
LEVELDB_VERSION=1.23

if [ ! -d leveldb-src ]; then
	git clone -b $LEVELDB_VERSION --depth 1 https://github.com/google/leveldb leveldb-src
	mkdir leveldb-src/build
fi

rm -rf leveldb

cd leveldb-src/build

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS="$OSX_FLAGS $OSX_ARCH" \
	-DCMAKE_CXX_FLAGS="$OSX_FLAGS $OSX_ARCH" \
	-DLEVELDB_BUILD_TESTS=FALSE \
	-DLEVELDB_BUILD_BENCHMARKS=FALSE \
	-DLEVELDB_INSTALL=FALSE

cmake --build . -j

mkdir -p ../../leveldb
cp -v libleveldb.a ../../leveldb
cp -rv ../include ../../leveldb/include

echo "LevelDB build successful"

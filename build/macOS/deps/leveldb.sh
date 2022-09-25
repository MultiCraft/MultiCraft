#!/bin/bash -e

. sdk.sh

if [ ! -d leveldb-src ]; then
	git clone --depth 1 https://github.com/google/leveldb leveldb-src
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
cp -r libleveldb.a ../../leveldb/libleveldb.a
cp -r ../include ../../leveldb/include

echo "LevelDB build successful"

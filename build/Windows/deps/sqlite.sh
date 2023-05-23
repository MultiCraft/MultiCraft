#!/bin/bash -e

SQLITE_VERSION=3.42.0

. ./sdk.sh

if [ ! -d sqlite-src ]; then
	wget https://www.sqlite.org/src/tarball/sqlite.tar.gz?r=version-$SQLITE_VERSION
	tar -xzvf sqlite.tar.gz?r=version-$SQLITE_VERSION
	mv sqlite sqlite-src
	rm sqlite.tar.gz?r=version-$SQLITE_VERSION
	mkdir sqlite-src/build
fi

cd sqlite-src/build

../configure \
	--enable-shared \
	--enable-static

make -j$NPROC

# update `include` folder
rm -rf ../../sqlite/include
mkdir -p ../../sqlite/include
cp -a ./sqlite3*.h ../../sqlite/include
# update lib
rm -rf ../../sqlite/lib
mkdir -p ../../sqlite/lib
cp .libs/libsqlite3.a ../../sqlite/lib

echo "SQLite build successful"

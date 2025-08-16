#!/bin/bash -e

SQLITE_VERSION=3.50.4

. ./sdk.sh

if [ ! -d sqlite-src ]; then
	git clone -b version-$SQLITE_VERSION --depth 1 https://github.com/sqlite/sqlite.git sqlite-src
	mkdir sqlite-src/build
fi

cd sqlite-src/build

../configure \
	--enable-shared \
	--enable-static

make -j${NPROC}

# update `include` folder
rm -rf ../../sqlite/include
mkdir -p ../../sqlite/include
cp -a ./sqlite3*.h ../../sqlite/include
# update lib
rm -rf ../../sqlite/lib
mkdir -p ../../sqlite/lib
cp ./libsqlite3.a ../../sqlite/lib

echo "SQLite build successful"

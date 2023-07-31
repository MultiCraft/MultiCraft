#!/bin/bash -e

LUAJIT_VERSION=2.1

. ./sdk.sh

if [ ! -d luajit-src ]; then
	wget https://github.com/LuaJIT/LuaJIT/archive/v$LUAJIT_VERSION.tar.gz
	tar -xzf v$LUAJIT_VERSION.tar.gz
	mv LuaJIT-$LUAJIT_VERSION luajit-src
	rm v$LUAJIT_VERSION.tar.gz
fi

cd luajit-src

make amalg -j${NPROC} BUILDMODE=static

# update `include` folder
rm -rf ../luajit/include
mkdir -p ../luajit/include
cp -a ./src/*.h ../luajit/include
# update lib
rm -rf ../luajit/lib
mkdir -p ../luajit/lib
cp src/libluajit.a ../luajit/lib

echo "LuaJIT build successful"

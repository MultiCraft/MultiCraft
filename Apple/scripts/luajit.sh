#!/bin/bash -e

LUAJIT_VERSION=2.1

. scripts/sdk.sh
mkdir -p deps; cd deps

[ ! -d luajit-src ] && \
	git clone -b v$LUAJIT_VERSION --depth 1 -c core.autocrlf=false https://github.com/LuaJIT/LuaJIT luajit-src

rm -rf luajit

cd luajit-src

for ARCH in x86_64 arm64
do
	echo "Building LuaJIT for $ARCH"
	make amalg -j \
		TARGET_FLAGS="$OSX_FLAGS -fno-fast-math -Wno-overriding-option -arch $ARCH"
	cp -v src/libluajit.a templib_$ARCH.a
	cp -v src/luajit.h luajit.h.tmp
	make clean
done

# repack into one .a
lipo -create templib_*.a -output libluajit.a
rm templib_*.a

mkdir -p ../luajit/include
cp -v src/*.h ../luajit/include
cp -v luajit.h.tmp ../luajit/include/luajit.h
cp -v libluajit.a ../luajit

echo "LuaJIT build successful"

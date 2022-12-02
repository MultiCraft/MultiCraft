#!/bin/bash -e

LUAJIT_VERSION="v2.1"

. scripts/sdk.sh
export MACOSX_DEPLOYMENT_TARGET=10.15
mkdir -p deps; cd deps

if [ ! -d luajit-src ]; then
	git clone -b $LUAJIT_VERSION --depth 1 -c core.autocrlf=false https://github.com/LuaJIT/LuaJIT luajit-src
fi

rm -rf luajit

cd luajit-src

for ARCH in x86_64 arm64
do
	echo "Building LuaJIT for $ARCH"
	make amalg -j \
		TARGET_FLAGS="$OSX_FLAGS -fno-fast-math -funwind-tables -fasynchronous-unwind-tables -arch $ARCH"
	cp src/libluajit.a templib_$ARCH.a
	make clean
done

# repack into one .a
lipo -create templib_*.a -output libluajit.a
rm templib_*.a

mkdir -p ../luajit/include
cp -v src/*.h ../luajit/include
cp -v libluajit.a ../luajit

echo "LuaJIT build successful"

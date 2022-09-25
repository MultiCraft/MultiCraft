#!/bin/bash -e

. sdk.sh
export MACOSX_DEPLOYMENT_TARGET=10.15

LUAJIT_VERSION=2.1

if [ ! -d LuaJIT-src ]; then
	wget https://github.com/LuaJIT/LuaJIT/archive/v$LUAJIT_VERSION.zip
	unzip v$LUAJIT_VERSION.zip
	mv LuaJIT-$LUAJIT_VERSION LuaJIT-src
	rm v$LUAJIT_VERSION.zip
fi

rm -rf LuaJIT

cd LuaJIT-src

for ARCH in x86_64 arm64
do
	echo "Building LuaJIT for $ARCH"
	make amalg -j \
		TARGET_FLAGS="$OSX_FLAGS -fno-fast-math -funwind-tables -fasynchronous-unwind-tables -arch $ARCH"
	mv src/libluajit.a templib_$ARCH.a
	make clean
done

# repack into one .a
lipo -create templib_*.a -output libluajit.a
rm templib_*.a

mkdir -p ../luajit/{lib,include}
cp -v src/*.h ../luajit/include
cp -v libluajit.a ../luajit/lib

echo "LuaJIT build successful"

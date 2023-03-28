#!/bin/bash -e

LUAJIT_VERSION=2.1

. sdk.sh

mkdir -p output/luajit/lib/$TARGET_ABI
mkdir -p deps; cd deps

if [ ! -d luajit-src ]; then
	wget https://github.com/LuaJIT/LuaJIT/archive/v$LUAJIT_VERSION.tar.gz
	tar -xzvf v$LUAJIT_VERSION.tar.gz
	mv LuaJIT-$LUAJIT_VERSION luajit-src
	rm v$LUAJIT_VERSION.tar.gz
fi

cd luajit-src

if [ $TARGET_ABI == armeabi-v7a ];
then
	HOST_CC="clang -m32"
else
	HOST_CC="clang -m64"
fi

make amalg -j \
	HOST_CC="$HOST_CC" \
	TARGET_SYS=Linux \
	CC="$CC" \
	TARGET_AR="$AR rcus" \
	TARGET_STRIP="$STRIP" \
	TARGET_FLAGS="$CFLAGS -fno-fast-math -Wno-undef-prefix" \
	BUILDMODE=static

# update `src` folder
rm -rf ../../output/luajit/include
mkdir -p ../../output/luajit/include
cp src/*.h ../../output/luajit/include/
# update lib
rm -rf ../../output/luajit/lib/$TARGET_ABI/libluajit.a
cp -r src/libluajit.a ../../output/luajit/lib/$TARGET_ABI/

echo "LuaJIT build successful"

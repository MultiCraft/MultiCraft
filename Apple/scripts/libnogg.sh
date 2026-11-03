#!/bin/bash -e

LIBNOGG_VERSION=1.18

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d libnogg-src ]; then
	git clone -b $LIBNOGG_VERSION --depth 1 https://github.com/achurch/libnogg libnogg-src
fi

rm -rf libnogg libnogg-src/build
mkdir libnogg-src/build
cd libnogg-src/build

for src in ../src/*/*.c; do
	$CC $OSX_FLAGS $OSX_ARCH -Xarch_arm64 -DENABLE_ASM_ARM_NEON -Xarch_x86_64 -DENABLE_ASM_X86_SSE2 \
		-std=c99 -DUSE_STDIO -DVERSION=\"$LIBNOGG_VERSION\" \
		-I.. -I../include -c $src -o $(basename $(dirname $src))-$(basename $src .c).o
done
xcrun libtool -static -o libnogg.a *.o

mkdir -p ../../libnogg
cp -v libnogg.a ../../libnogg
cp -rv ../include ../../libnogg/include

echo "libnogg build successful"

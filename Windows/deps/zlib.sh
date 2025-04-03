#!/bin/bash -e

ZLIB_VERSION=2.2.4

. ./sdk.sh

if [ ! -d zlib-src ]; then
	wget https://github.com/zlib-ng/zlib-ng/archive/$ZLIB_VERSION.tar.gz
	tar -xzf $ZLIB_VERSION.tar.gz
	mv zlib-ng-$ZLIB_VERSION zlib-src
	rm $ZLIB_VERSION.tar.gz
fi

cd zlib-src

mkdir -p build; cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DZLIB_COMPAT=1 \
    -DCMAKE_C_FLAGS="$CFLAGS"

cmake --build . -j${NPROC}

# update `include` folder
rm -rf ../../zlib/include
mkdir -p ../../zlib/include
cp -a ../*.h ../../zlib/include
cp -a *.h ../../zlib/include
# update lib
rm -rf ../../zlib/lib
mkdir -p ../../zlib/lib
cp libz.a ../../zlib/lib

echo "zlib build successful"

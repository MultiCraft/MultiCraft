#!/bin/bash -e

ZLIB_VERSION=1.2.13

. ./sdk.sh

if [ ! -d zlib-src ]; then
	wget https://github.com/madler/zlib/archive/v$ZLIB_VERSION.tar.gz
	tar -xzvf v$ZLIB_VERSION.tar.gz
	mv zlib-$ZLIB_VERSION zlib-src
	rm v$ZLIB_VERSION.tar.gz
fi

cd zlib-src

mkdir -p build; cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="$CFLAGS" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS -fPIC"

cmake --build . -j

# update `include` folder
rm -rf ../../zlib/include
mkdir -p ../../zlib/include
cp -a ../*.h ../../zlib/include
cp -a *.h ../../zlib/include
# update lib
rm -rf ../../zlib/lib
mkdir -p ../../zlib/lib
cp libzlibstatic.a ../../zlib/lib

echo "zlib build successful"

#!/bin/bash -e

. ./sdk.sh
PNG_VERSION=1.6.47

export DEPS_ROOT=$(pwd)

if [ ! -d libpng-src ]; then
	wget https://download.sourceforge.net/libpng/libpng-$PNG_VERSION.tar.gz
	tar -xzf libpng-$PNG_VERSION.tar.gz
	mv libpng-$PNG_VERSION libpng-src
	rm libpng-$PNG_VERSION.tar.gz
	mkdir libpng-src/build
fi

cd libpng-src/build

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DPNG_SHARED=OFF \
	-DPNG_TESTS=OFF \
	-DPNG_TOOLS=OFF \
	-DZLIB_ROOT="$DEPS_ROOT/zlib" \
	-DZLIB_INCLUDE_DIRS="$DEPS_ROOT/zlib/include" \
	-DCMAKE_C_FLAGS_RELEASE="$CFLAGS"

cmake --build . -j${NPROC}

# update `include` folder
rm -rf ../../libpng/include
mkdir -p ../../libpng/include
cp -a ../*.h ../../libpng/include
cp -a *.h ../../libpng/include
# update lib
rm -rf ../../libpng/lib
mkdir -p ../../libpng/lib
cp libpng16.a ../../libpng/lib

echo "libpng build successful"

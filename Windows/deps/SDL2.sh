#!/bin/bash -e

SDL2_VERSION=2.32.4

. ./sdk.sh

if [ ! -d SDL2-src ]; then
	wget https://github.com/libsdl-org/SDL/archive/release-$SDL2_VERSION.tar.gz
	tar -xzf release-$SDL2_VERSION.tar.gz
	mv SDL-release-$SDL2_VERSION sdl2-src
	rm release-$SDL2_VERSION.tar.gz
fi

cd sdl2-src

mkdir -p build; cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="$CFLAGS" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS -fPIC" \
    -DSDL_SHARED=0 \
    -DSDL_STATIC=1

cmake --build . -j${NPROC}

# update `include` folder
rm -rf ../../sdl2/include
mkdir -p ../../sdl2/include
cp -a ./include ../../sdl2
cp -a ./include-config-release/* ../../sdl2/include
# update lib
rm -rf ../../sdl2/lib
mkdir -p ../../sdl2/lib
cp -a *.a ../../sdl2/lib

echo "SDL2 build successful"

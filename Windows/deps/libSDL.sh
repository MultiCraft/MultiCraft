#!/bin/bash -e

SDL_VERSION=3.2.8

. ./sdk.sh

if [ ! -d sdl3-src ]; then
	wget https://github.com/libsdl-org/SDL/archive/release-$SDL_VERSION.tar.gz
	tar -xzf release-$SDL_VERSION.tar.gz
	mv SDL-release-$SDL_VERSION sdl3-src
	rm release-$SDL_VERSION.tar.gz
fi

cd sdl3-src

mkdir -p build; cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="$CFLAGS" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS -fPIC" \
    -DSDL_SHARED=0 \
    -DSDL_STATIC=1

cmake --build . -j${NPROC}

# update `include` folder
rm -rf ../../sdl3/include
mkdir -p ../../sdl3/include
cp -a ../include ../../sdl3
cp -a ./include-config-release/* ../../sdl3/include
# update lib
rm -rf ../../sdl3/lib
mkdir -p ../../sdl3/lib
cp -a *.a ../../sdl3/lib

echo "libSDL build successful"

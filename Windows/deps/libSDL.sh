#!/bin/bash -e

SDL_VERSION=3.2.20

. ./sdk.sh

if [ ! -d libSDL-src ]; then
	wget https://github.com/libsdl-org/SDL/archive/release-$SDL_VERSION.tar.gz
	tar -xzf release-$SDL_VERSION.tar.gz
	mv SDL-release-$SDL_VERSION libSDL-src
	rm release-$SDL_VERSION.tar.gz
fi

cd libSDL-src

mkdir -p build; cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="$CFLAGS" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS -fPIC" \
    -DSDL_SHARED=0 \
    -DSDL_STATIC=1

cmake --build . -j${NPROC}

# update `include` folder
rm -rf ../../libSDL/include
mkdir -p ../../libSDL/include
cp -a ../include ../../libSDL
cp -a ./include-config-release/* ../../libSDL/include
# update lib
rm -rf ../../libSDL/lib
mkdir -p ../../libSDL/lib
cp -a libSDL3.a ../../libSDL/lib/libSDL.a

echo "libSDL build successful"

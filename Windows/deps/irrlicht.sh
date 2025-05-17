#!/bin/bash -e

. ./sdk.sh

export DEPS_ROOT=$(pwd)

[ ! -d irrlicht-src ] && \
	git clone --depth 1 -b SDL3 https://github.com/deveee/Irrlicht irrlicht-src

cd irrlicht-src/source/Irrlicht

CPPFLAGS="$CPPFLAGS \
          -DNO_IRR_COMPILE_WITH_SDL_TEXTINPUT_ \
          -DNO_IRR_COMPILE_WITH_OGLES2_ \
          -DNO_IRR_COMPILE_WITH_DIRECT3D_9_ \
          -I$DEPS_ROOT/sdl3/include \
          -I$DEPS_ROOT/zlib/include \
          -I$DEPS_ROOT/libjpeg/include \
          -I$DEPS_ROOT/libpng/include \
          -I$DEPS_ROOT/openssl/include" \
CXXFLAGS="$CXXFLAGS -std=gnu++17" \
make staticlib_win32 -j${NPROC} NDEBUG=1

# update `include` folder
rm -rf ../../../irrlicht/include
mkdir -p ../../../irrlicht/include
cp -a ../../include ../../../irrlicht
# update lib
rm -rf ../../../irrlicht/lib
mkdir -p ../../../irrlicht/lib
cp ../../lib/Win32-gcc/libIrrlicht.a ../../../irrlicht/lib

echo "Irrlicht build successful"

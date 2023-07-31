#!/bin/bash -e

. ./sdk.sh
OGG_VERSION=1.3.5

if [ ! -d libogg-src ]; then
	git clone -b v$OGG_VERSION --depth 1 https://github.com/xiph/ogg libogg-src
	mkdir libogg-src/build
fi

cd libogg-src/build

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS_RELEASE="$CFLAGS" \
	-DCMAKE_CXX_FLAGS="$CXXFLAGS -fPIC"

cmake --build . -j${NPROC}

# update `include` folder
rm -rf ../../libogg/include
mkdir -p ../../libogg/include
cp -a ../include ../../libogg
cp -a ./include ../../libogg
# update lib
rm -rf ../../libogg/lib
mkdir -p ../../libogg/lib
cp libogg.a ../../libogg/lib

echo "Ogg build successful"

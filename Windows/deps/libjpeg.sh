#!/bin/bash -e

. ./sdk.sh
JPEG_VERSION=3.1.0

if [ ! -d libjpeg-src ]; then
	git clone -b $JPEG_VERSION --depth 1 https://github.com/libjpeg-turbo/libjpeg-turbo libjpeg-src
	mkdir libjpeg-src/build
fi

cd libjpeg-src/build

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DENABLE_SHARED=OFF \
	-DCMAKE_C_FLAGS_RELEASE="$CFLAGS"

cmake --build . -j${NPROC}

# update `include` folder
rm -rf ../../libjpeg/include
mkdir -p ../../libjpeg/include
cp -a ../src/*.h ../../libjpeg/include
cp -a *.h ../../libjpeg/include
# update lib
rm -rf ../../libjpeg/lib
mkdir -p ../../libjpeg/lib
cp libjpeg.a ../../libjpeg/lib

echo "libjpeg build successful"

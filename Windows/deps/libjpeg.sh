#!/bin/bash -e

. ./sdk.sh
JPEG_VERSION=3.0.0

if [ ! -d libjpeg-src ]; then
	wget https://download.sourceforge.net/libjpeg-turbo/libjpeg-turbo-$JPEG_VERSION.tar.gz
	tar -xzf libjpeg-turbo-$JPEG_VERSION.tar.gz
	mv libjpeg-turbo-$JPEG_VERSION libjpeg-src
	rm libjpeg-turbo-$JPEG_VERSION.tar.gz
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
cp -a ../*.h ../../libjpeg/include
cp -a *.h ../../libjpeg/include
# update lib
rm -rf ../../libjpeg/lib
mkdir -p ../../libjpeg/lib
cp libjpeg.a ../../libjpeg/lib

echo "libjpeg build successful"

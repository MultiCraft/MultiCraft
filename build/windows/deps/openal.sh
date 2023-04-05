#!/bin/bash -e

. ./sdk.sh
OPENAL_VERSION=1.22.2

if [ ! -d openal-src ]; then
	git clone -b $OPENAL_VERSION --depth 1 https://github.com/kcat/openal-soft openal-src
fi

cd openal-src/build

cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DLIBTYPE=STATIC \
	-DALSOFT_EMBED_HRTF_DATA=ON \
	-DALSOFT_UTILS=OFF \
	-DALSOFT_EXAMPLES=OFF \
	-DALSOFT_BACKEND_WAVE=OFF \
    -DCMAKE_C_FLAGS="$CFLAGS" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS -fPIC"

cmake --build . -j$NPROC

# update `include` folder
rm -rf ../../openal/include
mkdir -p ../../openal/include
cp -a ../include ../../openal
cp -a *.h ../../openal/include
# update lib
rm -rf ../../openal/lib
mkdir -p ../../openal/lib
cp libOpenAL32.a ../../openal/lib

echo "OpenAL-Soft build successful"

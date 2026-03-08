#!/bin/bash -e

NGHTTP2_VERSION=1.68.0

. scripts/sdk.sh
mkdir -p deps; cd deps

[ ! -d nghttp2-src ] && \
	git clone -b v$NGHTTP2_VERSION --depth 1 https://github.com/nghttp2/nghttp2 nghttp2-src

rm -rf nghttp2

cd nghttp2-src

for ARCH in x86_64 arm64
do
	echo "Building nghttp2 for $ARCH"
	mkdir -p build; cd build
	cmake .. \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_INSTALL_PREFIX="." \
		-DCMAKE_OSX_ARCHITECTURES=$ARCH \
		-DBUILD_SHARED_LIBS=0 \
		-DBUILD_STATIC_LIBS=1 \
		-DENABLE_APP=0 \
		-DENABLE_HPACK_TOOLS=0 \
		-DENABLE_EXAMPLES=0 \
		-DENABLE_FAILMALLOC=0 \
		-DENABLE_LIB_ONLY=1 \
		-DENABLE_DOC=0 \
		-DBUILD_TESTING=0

	cmake --build . -j

	if [ $ARCH = "x86_64" ]; then
		mkdir -p ../../nghttp2/include/nghttp2
		cp -r ../lib/includes/nghttp2/*.h ../../nghttp2/include/nghttp2
		cp -r lib/includes/nghttp2/*.h ../../nghttp2/include/nghttp2
	fi
	cp -v lib/libnghttp2.a ../../nghttp2/templib_$ARCH.a

	cd ..; rm -rf build
done

# repack into one .a
cd ../nghttp2
lipo -create templib_*.a -output libnghttp2.a
rm templib_*.a

echo "nghttp2 build successful"

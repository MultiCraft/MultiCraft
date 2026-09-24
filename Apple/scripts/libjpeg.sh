#!/bin/bash -e

JPEG_VERSION=3.2.0

. scripts/sdk.sh
mkdir -p deps; cd deps

if [ ! -d libjpeg-src ]; then
	git clone -b $JPEG_VERSION --depth 1 https://github.com/libjpeg-turbo/libjpeg-turbo libjpeg-src
	mkdir libjpeg-src/build
fi

rm -rf libjpeg

# NASM does not emit a platform load command, the linker warns about every .asm.o
NASM_PRELUDE="$PWD/nasm-macos.inc"
echo "%pragma macho build_version macos,${OSX_OSVER%%.*},${OSX_OSVER#*.}" > "$NASM_PRELUDE"

mkdir -p libjpeg/include

cd libjpeg-src

for ARCH in x86_64 arm64
do
	echo "Building libjpeg for $ARCH"
	mkdir -p build; cd build
	cmake .. \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS_RELEASE="$OSX_FLAGS -arch $ARCH" \
		-DCMAKE_INSTALL_PREFIX="." \
		-DCMAKE_OSX_ARCHITECTURES=$ARCH \
		-DENABLE_SHARED=OFF \
		-DREQUIRE_SIMD=ON \
		-DWITH_TOOLS=OFF \
		-DWITH_TURBOJPEG=OFF \
		-DCMAKE_ASM_NASM_FLAGS="-P$NASM_PRELUDE"

	cmake --build . -j
	make install -s

	if [ $ARCH = "x86_64" ]; then
		cp -rv include ../../libjpeg
		cp -v lib/libjpeg.a ../../libjpeg/templib_$ARCH.a
	else
		cp -v lib/libjpeg.a ../../libjpeg/templib_$ARCH.a
	fi

	cd ..; rm -rf build
done

# repack into one .a
cd ../libjpeg
lipo -create templib_*.a -output libjpeg.a
rm templib_*.a

echo "libjpeg build successful"

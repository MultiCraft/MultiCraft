#!/bin/bash -e

. sdk.sh

export ANDR_ROOT=$(pwd)

mkdir -p output/vorbis/lib/$TARGET_ABI
mkdir -p deps; cd deps

if [ ! -d vorbis-src ]; then
	git clone https://github.com/MoNTE48/libvorbis-android vorbis-src
fi

cd vorbis-src

$ANDROID_NDK/ndk-build -j \
	APP_ABI="$TARGET_ABI" \
	APP_PLATFORM=android-"$API" \
	APP_CFLAGS="$CFLAGS" \
	APP_STL="c++_static"

# update `include` folder
rm -rf ../../output/vorbis/include/
cp -r jni/include ../../output/vorbis/include
rm -rf ../../output/vorbis/include/dlg
# update lib
rm -rf ../../output/vorbis/lib/$TARGET_ABI/libvorbis.a
cp -r obj/local/$TARGET_ABI/libvorbis.a ../../output/vorbis/lib/$TARGET_ABI/libvorbis.a

echo "Vorbis build successful"

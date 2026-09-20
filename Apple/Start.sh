#!/bin/bash -e

echo
echo "Starting build MultiCraft for macOS..."

echo
echo "Build libraries:"

./scripts/libSDL.sh
./scripts/libjpeg.sh
./scripts/libpng.sh
./scripts/angle.sh
./scripts/irrlicht.sh
./scripts/gettext.sh
./scripts/freetype.sh bootstrap
./scripts/harfbuzz.sh
./scripts/freetype.sh
./scripts/leveldb.sh
./scripts/nghttp2.sh
./scripts/libcurl.sh
./scripts/libogg.sh
./scripts/libvorbis.sh
./scripts/luajit.sh
./scripts/openal.sh

echo
echo "All libraries were built!"

echo
echo "Preparing assets:"

./scripts/assets.sh

echo
echo "Preparing locales:"

./scripts/locale.sh

echo
echo "All done! You can continue in Xcode!"
open MultiCraft/MultiCraft.xcodeproj

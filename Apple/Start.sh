#!/bin/bash -e

echo
echo "Starting build MultiCraft for macOS..."

echo
echo "Build libraries:"

sh scripts/SDL2.sh
sh scripts/libjpeg.sh
sh scripts/libpng.sh
sh scripts/irrlicht.sh
sh scripts/gettext.sh
sh scripts/freetype.sh
sh scripts/leveldb.sh
sh scripts/libogg.sh
sh scripts/libvorbis.sh
sh scripts/luajit.sh
sh scripts/openal.sh

echo
echo "All libraries were built!"

echo
echo "Preparing assets:"

sh scripts/assets.sh

echo
echo "Preparing locales:"

sh scripts/locale.sh

echo
echo "All done! You can continue in Xcode!"
open MultiCraft/MultiCraft.xcodeproj

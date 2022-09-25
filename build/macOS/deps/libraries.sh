#!/bin/bash -e

sh SDL2.sh
sh libjpeg.sh
sh libpng.sh
sh irrlicht.sh
sh gettext.sh
sh freetype.sh
sh leveldb.sh
sh libogg.sh
sh libvorbis.sh
sh luajit.sh
sh openal.sh

echo
echo "All libraries were built!"

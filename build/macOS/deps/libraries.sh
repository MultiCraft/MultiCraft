#!/bin/bash -e

sh SDL2.sh
sh irrlicht.sh
sh gettext.sh
sh freetype.sh
sh luajit.sh
sh openal.sh

echo
echo "All libraries were built!"

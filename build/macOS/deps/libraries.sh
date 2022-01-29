#!/bin/bash -e

sh irrlicht.sh
sh gettext.sh
sh freetype.sh
sh luajit.sh
sh openal.sh

echo
echo "All libraries were built!"

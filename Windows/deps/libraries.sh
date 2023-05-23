#!/bin/bash -e

# Build libs

sh ./freetype.sh
sh ./gettext.sh
sh ./sqlite.sh
sh ./luajit.sh
sh ./libjpeg.sh
sh ./zlib.sh
sh ./libpng.sh
sh ./SDL2.sh
sh ./irrlicht.sh
sh ./openal.sh
sh ./libcurl.sh
sh ./libogg.sh
sh ./vorbis.sh

echo
echo "All libraries were built!"

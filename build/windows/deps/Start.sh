#!/bin/bash -e

# Build libs

sh ./freetype.sh
sh ./gettext.sh
sh ./leveldb.sh
sh ./luajit.sh
sh ./libjpeg.sh
sh ./libpng.sh
sh ./SDL2.sh
sh ./irrlicht.sh
sh ./openal.sh
sh ./mbedtls.sh
sh ./libcurl.sh
sh ./libogg.sh
sh ./vorbis.sh

echo "Done!"

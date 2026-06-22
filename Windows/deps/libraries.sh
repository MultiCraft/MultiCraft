#!/bin/bash -e

# Build libs

sh ./zlib.sh
sh ./libpng.sh
sh ./freetype.sh bootstrap
sh ./harfbuzz.sh
sh ./freetype.sh
sh ./gettext.sh
sh ./sqlite.sh
sh ./luajit.sh
sh ./libjpeg.sh
sh ./openssl.sh
sh ./libSDL.sh
sh ./irrlicht.sh
sh ./openal.sh
sh ./libcurl.sh
sh ./libogg.sh
sh ./vorbis.sh
sh ./zstd.sh

echo
echo "All libraries were built!"

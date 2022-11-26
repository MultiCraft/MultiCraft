#!/bin/bash -e

ALL_FONTS=true

if [ ! -d MultiCraft/MultiCraft.xcodeproj ]; then
	echo "Run this in build/macOS"
	exit 1
fi

DEST=$(pwd)/assets

mkdir -p $DEST/fonts

if $ALL_FONTS
then
	cp ../fonts/*.ttf $DEST/fonts/
else
  cp ../fonts/MultiCraftFont.ttf $DEST/fonts/
fi

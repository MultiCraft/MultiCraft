#!/bin/bash -e

ALL_FONTS=false

if [ ! -d MultiCraft/MultiCraft.xcodeproj ]; then
	echo "Run this from Apple folder"
	exit 1
fi

DEST=$(pwd)/assets

mkdir -p $DEST/fonts

if $ALL_FONTS
then
	cp ../fonts/*.ttf $DEST/fonts/
else
	cp ../fonts/MultiCraftFont.ttf $DEST/fonts/
	cp ../fonts/NotoEmoji-Regular.ttf $DEST/fonts/
fi

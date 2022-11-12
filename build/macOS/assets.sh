#!/bin/bash -e

if [ ! -d MultiCraft/MultiCraft.xcodeproj ]; then
	echo "Run this in build/macOS"
	exit 1
fi

DEST=$(pwd)/assets

mkdir -p $DEST/fonts
cp ../../fonts/*.ttf $DEST/fonts/

#!/bin/bash -e

# Generates different AppIcon images with correct dimensions
# (brew package: imagemagick)
# (install: brew install imagemagick)
SIZES="16 32 64 128 256 512 1024"
SRCFILE=icon.png
DSTDIR=MultiCraft/MultiCraft/Assets.xcassets/AppIcon.appiconset

for sz in $SIZES; do
	echo "Creating ${sz}x${sz} icon"
	convert -resize ${sz}x${sz} $SRCFILE $DSTDIR/AppIcon-${sz}.png
done

echo "App Icon create successful"

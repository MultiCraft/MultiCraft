#!/bin/bash -e

if [ ! -d MultiCraft/MultiCraft.xcodeproj ]; then
	echo "Run this in build/macOS"
	exit 1
fi

DEST=$(pwd)

pushd ../../po
for lang in *; do
	[ ${#lang} -ne 2 ] && continue
	mopath=$DEST/locale/$lang/LC_MESSAGES
	mkdir -p $mopath
	pushd $lang
	for fn in *.po; do
		# brew install gettext
		msgfmt -o $mopath/${fn/.po/.mo} $fn
	done
	popd
done
popd

find $DEST -type d -name '.git' -print0 | xargs -0 -- rm -r
find $DEST -type f -name '.*' -delete

#!/bin/bash -e

if [ ! -d MultiCraft/MultiCraft.xcodeproj ]; then
	echo "Run this from Apple folder"
	exit 1
fi

DEST=$(pwd)/assets/locale
broken_langs=(fil gd gl dv eo he hi jbo kn ko kk ky ms_Arab nn pt_BR sr_Cyrl sr_Latn zh_CN zh_TW)

pushd ../po
for lang in *; do
	[ ${#lang} -ne 2 ] && continue
	# Skip broken languages
	if [[ " ${broken_langs[@]} " =~ " ${lang} " ]]; then
		continue
	fi
	mopath=$DEST/$lang/LC_MESSAGES
	mkdir -p $mopath
	pushd $lang
	for fn in *.po; do
		# brew install gettext
		msgfmt -o $mopath/${fn/.po/.mo} $fn
	done
	popd
done
popd

# Remove hidden files and directories
find $DEST -type d,f -name '.*' -print0 | xargs -0 -- rm -rf

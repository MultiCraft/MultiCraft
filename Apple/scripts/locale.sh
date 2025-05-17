#!/bin/bash -e

if [ ! -d MultiCraft/MultiCraft.xcodeproj ]; then
	echo "Run this from Apple folder"
	exit 1
fi

DEST=$(pwd)/assets/locale
broken_langs=(fil gd gl dv eo he hi jbo kn kk ky ms_Arab nn pt_BR sr_Cyrl sr_Latn zh_TW minetest.pot)

pushd ../po
for lang in *; do
	# Skip broken languages
	if [[ " ${broken_langs[@]} " =~ " ${lang} " ]]; then
		continue
	fi
	mopath=$DEST/$lang/LC_MESSAGES
	mkdir -p $mopath
	pushd $lang
	for fn in *.po; do
		# brew install gettext
		if [[ $lang == zh_CN && $fn == minetest.po ]]; then
			# special case for zh_CN language
			sed -e 's/msgstr "zh_CN"/msgstr "zh"/g' -e 's/Chinese (Simplified)/Chinese/g' $fn | msgfmt - -o $mopath/${fn/.po/.mo}
		else
			msgfmt -o $mopath/${fn/.po/.mo} $fn
		fi
	done
	popd
done
popd

# special case for zh_CN language
mv $DEST/zh_CN $DEST/zh

# Remove hidden files and directories
find $DEST -type d,f -name '.*' -print0 | xargs -0 -- rm -rf

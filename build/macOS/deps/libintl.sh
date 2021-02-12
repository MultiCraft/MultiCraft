#!/bin/bash -e

if [ ! -d libintl ]; then
	wget https://github.com/MoNTE48/libintl-lite/archive/master.zip
	unzip master.zip
	mv libintl-lite-master libintl
	rm master.zip
fi

echo "libintl-lite downloaded successful"

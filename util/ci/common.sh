#!/bin/bash -e

# Linux build only
install_linux_deps() {
	local pkgs=(cmake git libsdl2-dev libbz2-dev libpng-dev \
		libjpeg-dev libxxf86vm-dev libgl1-mesa-dev libsqlite3-dev \
		libhiredis-dev libogg-dev libgmp-dev libvorbis-dev libopenal-dev \
		gettext libpq-dev libleveldb-dev libcurl4-openssl-dev libzstd-dev)

	sudo apt-get update
	sudo apt-get install -y --no-install-recommends ${pkgs[@]} "$@"
}

# Mac OSX build only
install_macosx_deps() {
	brew update
	brew install freetype gettext hiredis irrlicht leveldb libogg libvorbis luajit
	if brew ls | grep -q jpeg; then
		brew upgrade jpeg
	else
		brew install jpeg
	fi
	#brew upgrade postgresql
}

# Build linux dependencies
build_linux_deps() {
	mkdir deps
	cd deps
	git clone --depth 1 -b SDL2 https://github.com/MoNTE48/Irrlicht
	cd Irrlicht/source/Irrlicht
	make -j
	cd ../../../
}

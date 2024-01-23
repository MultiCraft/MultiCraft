#!/bin/bash
set -e

CORE_GIT=https://github.com/minetest/minetest
CORE_BRANCH=master
CORE_NAME=minetest
GAME_GIT=https://github.com/minetest/minetest_game
GAME_BRANCH=master
GAME_NAME=minetest_game

dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [ $# -ne 1 ]; then
	echo "Usage: $0 <build directory>"
	exit 1
fi
builddir=$1
mkdir -p $builddir
builddir="$( cd "$builddir" && pwd )"
packagedir=$builddir/packages
libdir=$builddir/libs

toolchain_file=$dir/toolchain_x86_64-w64-mingw32.cmake
irrlicht_version=1.8.4
ogg_version=1.3.5
vorbis_version=1.3.7
curl_version=8.0.1
gettext_version=0.20.2
freetype_version=2.12.1
sqlite3_version=3.41.2
luajit_version=20230221
leveldb_version=1.23
zlib_version=1.2.13
sdl_version=2.28.5
jpeg_version=3.0.0
png_version=1.6.40

mkdir -p $packagedir
mkdir -p $libdir

cd $builddir

# Get stuff
# [ -e $packagedir/irrlicht-$irrlicht_version.zip ] || wget http://minetest.kitsunemimi.pw/irrlicht-$irrlicht_version-win64.zip \
#	-c -O $packagedir/irrlicht-$irrlicht_version.zip
[ -e $packagedir/zlib-$zlib_version.zip ] || wget http://minetest.kitsunemimi.pw/zlib-$zlib_version-win64.zip \
	-c -O $packagedir/zlib-$zlib_version.zip
[ -e $packagedir/libogg-$ogg_version.zip ] || wget http://minetest.kitsunemimi.pw/libogg-$ogg_version-win64.zip \
	-c -O $packagedir/libogg-$ogg_version.zip
[ -e $packagedir/libvorbis-$vorbis_version.zip ] || wget http://minetest.kitsunemimi.pw/libvorbis-$vorbis_version-win64.zip \
	-c -O $packagedir/libvorbis-$vorbis_version.zip
[ -e $packagedir/curl-$curl_version.zip ] || wget http://minetest.kitsunemimi.pw/curl-$curl_version-win64.zip \
	-c -O $packagedir/curl-$curl_version.zip
[ -e $packagedir/gettext-$gettext_version.zip ] || wget http://minetest.kitsunemimi.pw/gettext-$gettext_version-win64.zip \
	-c -O $packagedir/gettext-$gettext_version.zip
[ -e $packagedir/freetype-$freetype_version.zip ] || wget http://minetest.kitsunemimi.pw/freetype-$freetype_version-win64.zip \
	-c -O $packagedir/freetype-$freetype_version.zip
[ -e $packagedir/sqlite3-$sqlite3_version.zip ] || wget http://minetest.kitsunemimi.pw/sqlite3-$sqlite3_version-win64.zip \
	-c -O $packagedir/sqlite3-$sqlite3_version.zip
[ -e $packagedir/luajit-$luajit_version.zip ] || wget http://minetest.kitsunemimi.pw/luajit-$luajit_version-win64.zip \
	-c -O $packagedir/luajit-$luajit_version.zip
[ -e $packagedir/libleveldb-$leveldb_version.zip ] || wget http://minetest.kitsunemimi.pw/libleveldb-$leveldb_version-win64.zip \
	-c -O $packagedir/libleveldb-$leveldb_version.zip
[ -e $packagedir/openal_stripped.zip ] || wget http://minetest.kitsunemimi.pw/openal_stripped64.zip \
	-c -O $packagedir/openal_stripped.zip
[ -e $packagedir/SDL2-devel-$sdl_version-mingw.zip ] || wget https://github.com/libsdl-org/SDL/releases/download/release-$sdl_version/SDL2-devel-$sdl_version-mingw.zip \
	-c -O $packagedir/SDL2-devel-$sdl_version-mingw.zip


# Extract stuff
cd $libdir
# [ -d irrlicht ] || unzip -o $packagedir/irrlicht-$irrlicht_version.zip -d irrlicht
[ -d zlib ] || unzip -o $packagedir/zlib-$zlib_version.zip -d zlib
[ -d libogg ] || unzip -o $packagedir/libogg-$ogg_version.zip -d libogg
[ -d libvorbis ] || unzip -o $packagedir/libvorbis-$vorbis_version.zip -d libvorbis
[ -d libcurl ] || unzip -o $packagedir/curl-$curl_version.zip -d libcurl
[ -d gettext ] || unzip -o $packagedir/gettext-$gettext_version.zip -d gettext
[ -d freetype ] || unzip -o $packagedir/freetype-$freetype_version.zip -d freetype
[ -d sqlite3 ] || unzip -o $packagedir/sqlite3-$sqlite3_version.zip -d sqlite3
[ -d openal_stripped ] || unzip -o $packagedir/openal_stripped.zip
[ -d luajit ] || unzip -o $packagedir/luajit-$luajit_version.zip -d luajit
[ -d leveldb ] || unzip -o $packagedir/libleveldb-$leveldb_version.zip -d leveldb
[ -d SDL2 ] || (unzip -o $packagedir/SDL2-devel-$sdl_version-mingw.zip SDL2-$sdl_version/x86_64-w64-mingw32/* -d SDL2 && mv SDL2/SDL2-$sdl_version/x86_64-w64-mingw32/* SDL2)

# Get libjpeg
if [ ! -d libjpeg ]; then
	wget https://download.sourceforge.net/libjpeg-turbo/libjpeg-turbo-$jpeg_version.tar.gz
	tar -xzf libjpeg-turbo-$jpeg_version.tar.gz
	mv libjpeg-turbo-$jpeg_version libjpeg
	rm libjpeg-turbo-$jpeg_version.tar.gz
	mkdir libjpeg/build
	cd libjpeg/build

	cmake .. \
		-DCMAKE_TOOLCHAIN_FILE=$toolchain_file \
		-DCMAKE_SYSTEM_PROCESSOR=AMD64 \
		-DCMAKE_INSTALL_PREFIX=/tmp \
		-DCMAKE_BUILD_TYPE=Release \
		-DENABLE_SHARED=OFF \
		-DCMAKE_C_FLAGS_RELEASE="$CFLAGS"
	
	cmake --build . -j$(nproc)
	
	cd -
fi

# Get libpng
if [ ! -d libpng ]; then
	wget https://download.sourceforge.net/libpng/libpng-$png_version.tar.gz
	tar -xzf libpng-$png_version.tar.gz
	mv libpng-$png_version libpng
	rm libpng-$png_version.tar.gz
	mkdir libpng/build
	cd libpng/build
	
	cmake .. \
		-DCMAKE_TOOLCHAIN_FILE=$toolchain_file \
		-DCMAKE_BUILD_TYPE=Release \
		-DPNG_SHARED=OFF \
		-DPNG_TESTS=OFF \
		-DPNG_EXECUTABLES=OFF \
		-DPNG_BUILD_ZLIB=ON \
		-DZLIB_INCLUDE_DIRS="$libdir/zlib/include" \
		-DCMAKE_C_FLAGS_RELEASE="$CFLAGS"
	
	cmake --build . -j$(nproc)
	
	cd -
fi

# Get irrlicht
if [ ! -d irrlicht ]; then
	git clone --depth 1 -b SDL2 https://github.com/MoNTE48/Irrlicht irrlicht
	
	cd irrlicht/source/Irrlicht

	CC="x86_64-w64-mingw32-gcc" \
	CXX="x86_64-w64-mingw32-g++" \
	CPPFLAGS="$CPPFLAGS \
		-DCMAKE_TOOLCHAIN_FILE=$toolchain_file \
		-DNO_IRR_COMPILE_WITH_SDL_TEXTINPUT_ \
		-DNO_IRR_COMPILE_WITH_OGLES2_ \
		-DNO_IRR_COMPILE_WITH_DIRECT3D_9_ \
		-I/usr/x86_64-w64-mingw32/include \
		-I$libdir/SDL2/include/SDL2 \
		-I$libdir/zlib/include \
		-I$libdir/libjpeg \
		-I$libdir/libjpeg/build \
		-I$libdir/libpng \
		-I$libdir/libpng/build" \
	CXXFLAGS="$CXXFLAGS -std=gnu++17" \
	make staticlib_win32 -j$(nproc) NDEBUG=1
	
	cd -
fi

# Get minetest
cd $builddir
if [ ! "x$EXISTING_MINETEST_DIR" = "x" ]; then
	cd /$EXISTING_MINETEST_DIR # must be absolute path
else
	[ -d $CORE_NAME ] && (cd $CORE_NAME && git pull) || (git clone -b $CORE_BRANCH $CORE_GIT)
	cd $CORE_NAME
fi
git_hash=$(git rev-parse --short HEAD)

# Get minetest_game
if [ "x$NO_MINETEST_GAME" = "x" ]; then
	cd games
	[ -d $GAME_NAME ] && (cd $GAME_NAME && git pull) || (git clone -b $GAME_BRANCH $GAME_GIT)
	cd ..
fi

# Build the thing
[ -d _build ] && rm -Rf _build/
mkdir _build
cd _build
cmake .. \
	-DCMAKE_TOOLCHAIN_FILE=$toolchain_file \
	-DCMAKE_INSTALL_PREFIX=/tmp \
	-DVERSION_EXTRA=$git_hash \
	-DBUILD_CLIENT=1 -DBUILD_SERVER=0 \
	\
	-DENABLE_SOUND=1 \
	-DENABLE_CURL=1 \
	-DENABLE_GETTEXT=1 \
	-DENABLE_FREETYPE=1 \
	-DENABLE_LEVELDB=1 \
	\
	-DUSE_STATIC_BUILD=1 \
	-DUSE_SDL=1 \
	-DSDL2_LIBRARIES="$libdir/SDL2/lib/libSDL2.a" \
	-DSDL2_INCLUDE_DIRS="$libdir/SDL2/include/SDL2" \
	\
	-DCMAKE_C_FLAGS=" \
		-DNO_IRR_COMPILE_WITH_SDL_TEXTINPUT_ \
		-DNO_IRR_COMPILE_WITH_OGLES2_ \
		-DNO_IRR_COMPILE_WITH_DIRECT3D_9_ \
		-D_IRR_STATIC_LIB_" \
	-DCMAKE_CXX_FLAGS=" \
		-DNO_IRR_COMPILE_WITH_SDL_TEXTINPUT_ \
		-DNO_IRR_COMPILE_WITH_OGLES2_ \
		-DNO_IRR_COMPILE_WITH_DIRECT3D_9_ \
		-D_IRR_STATIC_LIB_" \
	-DIRRLICHT_INCLUDE_DIR=$libdir/irrlicht/include \
	-DIRRLICHT_LIBRARY=$libdir/irrlicht/lib/Win32-gcc/libIrrlicht.a \
	\
	-DZLIB_INCLUDE_DIR=$libdir/zlib/include \
	-DZLIB_LIBRARIES=$libdir/zlib/lib/libz.a \
	\
	-DPNG_INCLUDE_DIR="$libdir/libpng/include" \
	-DPNG_LIBRARIES="$libdir/libpng/build/libpng16.a" \
	\
	-DJPEG_INCLUDE_DIR="$libdir/libjpeg/include" \
	-DJPEG_LIBRARIES="$libdir/libjpeg/build/libjpeg.a" \
	\
	-DLUA_INCLUDE_DIR=$libdir/luajit/include \
	-DLUA_LIBRARY=$libdir/luajit/libluajit.a \
	\
	-DOGG_INCLUDE_DIR=$libdir/libogg/include \
	-DOGG_LIBRARY=$libdir/libogg/lib/libogg.dll.a \
	-DOGG_DLL=$libdir/libogg/bin/libogg-0.dll \
	\
	-DVORBIS_INCLUDE_DIR=$libdir/libvorbis/include \
	-DVORBIS_LIBRARY=$libdir/libvorbis/lib/libvorbis.dll.a \
	-DVORBIS_DLL=$libdir/libvorbis/bin/libvorbis-0.dll \
	-DVORBISFILE_LIBRARY=$libdir/libvorbis/lib/libvorbisfile.dll.a \
	-DVORBISFILE_DLL=$libdir/libvorbis/bin/libvorbisfile-3.dll \
	\
	-DOPENAL_INCLUDE_DIR=$libdir/openal_stripped/include/AL \
	-DOPENAL_LIBRARY=$libdir/openal_stripped/lib/libOpenAL32.dll.a \
	-DOPENAL_DLL=$libdir/openal_stripped/bin/OpenAL32.dll \
	\
	-DCURL_DLL=$libdir/libcurl/bin/libcurl-4.dll \
	-DCURL_INCLUDE_DIR=$libdir/libcurl/include \
	-DCURL_LIBRARY=$libdir/libcurl/lib/libcurl.dll.a \
	\
	-DGETTEXT_MSGFMT=`which msgfmt` \
	-DGETTEXT_DLL=$libdir/gettext/bin/libintl-8.dll \
	-DGETTEXT_ICONV_DLL=$libdir/gettext/bin/libiconv-2.dll \
	-DGETTEXT_INCLUDE_DIR=$libdir/gettext/include \
	-DGETTEXT_LIBRARY=$libdir/gettext/lib/libintl.dll.a \
	\
	-DFREETYPE_INCLUDE_DIR_freetype2=$libdir/freetype/include/freetype2 \
	-DFREETYPE_INCLUDE_DIR_ft2build=$libdir/freetype/include/freetype2 \
	-DFREETYPE_LIBRARY=$libdir/freetype/lib/libfreetype.dll.a \
	-DFREETYPE_DLL=$libdir/freetype/bin/libfreetype-6.dll \
	\
	-DSQLITE3_INCLUDE_DIR=$libdir/sqlite3/include \
	-DSQLITE3_LIBRARY=$libdir/sqlite3/lib/libsqlite3.dll.a \
	-DSQLITE3_DLL=$libdir/sqlite3/bin/libsqlite3-0.dll \
	\
	-DLEVELDB_INCLUDE_DIR=$libdir/leveldb/include \
	-DLEVELDB_LIBRARY=$libdir/leveldb/lib/libleveldb.dll.a \
	-DLEVELDB_DLL=$libdir/leveldb/bin/libleveldb.dll

make -j$(nproc)

[ "x$NO_PACKAGE" = "x" ] && make package

exit 0
# EOF

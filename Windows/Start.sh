#!/bin/bash -e

echo
echo "Starting build MultiCraft for Windows..."

echo
echo "Build Libraries:"

cd deps
sh libraries.sh
cd ..

export DEPS_ROOT=$(pwd)/deps

cmake ../ \
	-DCMAKE_BUILD_TYPE=Release \
	-DENABLE_SQLITE=1 \
	-DENABLE_POSTGRESQL=0 \
	-DENABLE_LEVELDB=0 \
	-DENABLE_REDIS=0 \
	-DENABLE_SPATIAL=0 \
	-DENABLE_PROMETHEUS=0 \
	-DENABLE_CURSES=0 \
	-DENABLE_SYSTEM_GMP=0 \
	-DUSE_SDL=1 \
	-DUSE_STATIC_BUILD=1 \
	-DCMAKE_C_FLAGS="-static \
		-DNO_IRR_COMPILE_WITH_SDL_TEXTINPUT_ \
		-DNO_IRR_COMPILE_WITH_OGLES2_ \
		-DNO_IRR_COMPILE_WITH_DIRECT3D_9_ \
		-DNO_IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_ \
		-D_IRR_STATIC_LIB_ \
		-DAL_LIBTYPE_STATIC \
		-DCURL_STATICLIB \
		-D_WIN32_WINNT=0x0600" \
	-DCMAKE_CXX_FLAGS="-static \
		-DNO_IRR_COMPILE_WITH_SDL_TEXTINPUT_ \
		-DNO_IRR_COMPILE_WITH_OGLES2_ \
		-DNO_IRR_COMPILE_WITH_DIRECT3D_9_ \
		-DNO_IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_ \
		-D_IRR_STATIC_LIB_ \
		-DAL_LIBTYPE_STATIC \
		-DCURL_STATICLIB \
		-D_WIN32_WINNT=0x0600" \
	-DIRRLICHT_LIBRARY="$DEPS_ROOT/irrlicht/lib/libIrrlicht.a" \
	-DIRRLICHT_INCLUDE_DIR="$DEPS_ROOT/irrlicht/include" \
	-DSDL2_LIBRARIES="$DEPS_ROOT/sdl2/lib/libSDL2.a" \
	-DSDL2_INCLUDE_DIRS="$DEPS_ROOT/sdl2/include" \
	-DCURL_LIBRARY="$DEPS_ROOT/libcurl/lib/libcurl.a" \
	-DCURL_INCLUDE_DIR="$DEPS_ROOT/libcurl/include" \
	-DLUA_LIBRARY="$DEPS_ROOT/luajit/lib/libluajit.a" \
	-DLUA_INCLUDE_DIR="$DEPS_ROOT/luajit/include" \
	-DZLIB_LIBRARIES="$DEPS_ROOT/zlib/lib/libz.a" \
	-DZLIB_INCLUDE_DIR="$DEPS_ROOT/zlib/include" \
	-DPNG_LIBRARIES="$DEPS_ROOT/libpng/lib/libpng16.a" \
	-DPNG_INCLUDE_DIR="$DEPS_ROOT/libpng/include" \
	-DJPEG_LIBRARIES="$DEPS_ROOT/libjpeg/lib/libjpeg.a" \
	-DJPEG_INCLUDE_DIR="$DEPS_ROOT/libjpeg/include" \
	-DFREETYPE_LIBRARY="$DEPS_ROOT/freetype/lib/libfreetype.a" \
	-DFREETYPE_INCLUDE_DIRS="$DEPS_ROOT/freetype/include" \
	-DSQLITE3_LIBRARY="$DEPS_ROOT/sqlite/lib/libsqlite3.a" \
	-DSQLITE3_INCLUDE_DIR="$DEPS_ROOT/sqlite/include" \
	-DOGG_LIBRARY="$DEPS_ROOT/libogg/lib/libogg.a" \
	-DOGG_INCLUDE_DIR="$DEPS_ROOT/libogg/include" \
	-DVORBIS_LIBRARY="$DEPS_ROOT/libvorbis/lib/libvorbis.a" \
	-DVORBISFILE_LIBRARY="$DEPS_ROOT/libvorbis/lib/libvorbisfile.a" \
	-DVORBIS_INCLUDE_DIR="$DEPS_ROOT/libvorbis/include" \
	-DGETTEXT_LIBRARY="$DEPS_ROOT/gettext/lib/libintl.a" \
	-DGETTEXT_ICONV_LIBRARY="/mingw64/lib/libiconv.a" \
	-DGETTEXT_INCLUDE_DIR="$DEPS_ROOT/gettext/include" \
	-DOPENAL_LIBRARY="$DEPS_ROOT/openal/lib/libOpenAL32.a" \
	-DOPENAL_INCLUDE_DIR="$DEPS_ROOT/openal/include/AL"

echo
echo "Build with 'cmake --build . -j'"

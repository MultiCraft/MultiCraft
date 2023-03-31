#!/bin/bash -e

echo
echo "Starting build MultiCraft for Windows..."

echo
echo "Build Libraries:"

cd deps
sh libraries.sh
cd ..

export DEPS_ROOT=$(pwd)/deps

cmake ../../ \
	-DCMAKE_BUILD_TYPE=Release \
	-DUSE_SDL=1 \
	-DCMAKE_C_FLAGS="-D_IRR_COMPILE_WITH_SDL_DEVICE_ \
		-DNO_IRR_COMPILE_WITH_SDL_TEXTINPUT_ \
		-DNO_IRR_COMPILE_WITH_OGLES2_ \
		-DNO_IRR_COMPILE_WITH_DIRECT3D_9_ \
		-DNO_IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_ \
		-D_IRR_STATIC_LIB_ \
		-DAL_LIBTYPE_STATIC" \
	-DCMAKE_CXX_FLAGS="-D_IRR_COMPILE_WITH_SDL_DEVICE_ \
		-DNO_IRR_COMPILE_WITH_SDL_TEXTINPUT_ \
		-DNO_IRR_COMPILE_WITH_OGLES2_ \
		-DNO_IRR_COMPILE_WITH_DIRECT3D_9_ \
		-DNO_IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_ \
		-D_IRR_STATIC_LIB_ \
		-DAL_LIBTYPE_STATIC" \
	-DIRRLICHT_LIBRARY="$DEPS_ROOT/irrlicht/lib/libIrrlicht.a" \
	-DIRRLICHT_INCLUDE_DIR="$DEPS_ROOT/irrlicht/include" \
	-DSDL2_LIBRARIES="$DEPS_ROOT/sdl2/lib/libSDL2.a" \
	-DSDL2_INCLUDE_DIR="$DEPS_ROOT/sdl2/include" \
	-DLUA_LIBRARY="$DEPS_ROOT/luajit/lib/libluajit.a" \
	-DLUA_INCLUDE_DIR="$DEPS_ROOT/luajit/include" \
	-DZLIB_LIBRARIES="$DEPS_ROOT/zlib/lib/libzlibstatic.a" \
	-DZLIB_INCLUDE_DIR="$DEPS_ROOT/zlib/include" \
	-DPNG_LIBRARY="$DEPS_ROOT/libpng/lib/libpng16.a" \
	-DPNG_INCLUDE_DIR="$DEPS_ROOT/libpng/include" \
	-DJPEG_LIBRARY="$DEPS_ROOT/libjpeg/lib/libjpeg.a" \
	-DJPEG_INCLUDE_DIR="$DEPS_ROOT/libjpeg/include" \
	-DFREETYPE_LIBRARY="$DEPS_ROOT/freetype/lib/libfreetype.a" \
	-DFREETYPE_INCLUDE_DIRS="$DEPS_ROOT/freetype/include" \
	-DLEVELDB_LIBRARY="$DEPS_ROOT/leveldb/lib/libleveldb.a" \
	-DLEVELDB_INCLUDE_DIR="$DEPS_ROOT/leveldb/include" \
	-DOGG_LIBRARY="$DEPS_ROOT/libogg/lib/libogg.a" \
	-DOGG_INCLUDE_DIR="$DEPS_ROOT/libogg/include" \
	-DVORBIS_LIBRARY="$DEPS_ROOT/libvorbis/lib/libvorbis.a" \
	-DVORBISFILE_LIBRARY="$DEPS_ROOT/libvorbis/lib/libvorbisfile.a" \
	-DVORBIS_INCLUDE_DIR="$DEPS_ROOT/libvorbis/include" \
	-DGETTEXT_DLL="$DEPS_ROOT/gettext/lib/libintl.a" \
	-DGETTEXT_INCLUDE_DIR="$DEPS_ROOT/gettext/include" \
	-DOPENAL_LIBRARY="$DEPS_ROOT/openal/lib/libOpenAL32.a" \
	-DOPENAL_INCLUDE_DIR="$DEPS_ROOT/openal/include/AL"

echo "Build with 'cmake --build . -j'"

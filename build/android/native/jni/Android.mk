LOCAL_PATH := $(call my-dir)/..

#LOCAL_ADDRESS_SANITIZER:=true

include $(CLEAR_VARS)
LOCAL_MODULE := Curl
LOCAL_SRC_FILES := deps/Android/Curl/clang/$(APP_ABI)/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := Freetype
LOCAL_SRC_FILES := deps/Android/Freetype/clang/$(APP_ABI)/libfreetype.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := Irrlicht
LOCAL_SRC_FILES := deps/Android/Irrlicht/clang/$(APP_ABI)/libIrrlicht.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libpng
LOCAL_SRC_FILES := deps/Android/libpng/clang/$(APP_ABI)/libpng.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjpeg
LOCAL_SRC_FILES := deps/Android/libjpeg/clang/$(APP_ABI)/libjpeg.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := SDL2
LOCAL_SRC_FILES := deps/Android/SDL2/clang/$(APP_ABI)/libSDL2.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := LevelDB
LOCAL_SRC_FILES := deps/Android/LevelDB/clang/$(APP_ABI)/libleveldb.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := LuaJIT
LOCAL_SRC_FILES := deps/Android/LuaJIT/clang/$(APP_ABI)/libluajit.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mbedTLS
LOCAL_SRC_FILES := deps/Android/mbedTLS/clang/$(APP_ABI)/libmbedtls.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mbedx509
LOCAL_SRC_FILES := deps/Android/mbedTLS/clang/$(APP_ABI)/libmbedx509.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mbedcrypto
LOCAL_SRC_FILES := deps/Android/mbedTLS/clang/$(APP_ABI)/libmbedcrypto.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := OpenAL
LOCAL_SRC_FILES := deps/Android/OpenAL-Soft/clang/$(APP_ABI)/libopenal.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := Gettext
LOCAL_SRC_FILES := deps/Android/Gettext/clang/$(APP_ABI)/libintl.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := Vorbis
LOCAL_SRC_FILES := deps/Android/Vorbis/clang/$(APP_ABI)/libvorbis.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := MultiCraft

LOCAL_CFLAGS += \
	-DJSONCPP_NO_LOCALE_SUPPORT     \
	-DHAVE_TOUCHSCREENGUI           \
	-DENABLE_GLES=1                 \
	-DUSE_CURL=1                    \
	-DUSE_SOUND=1                   \
	-DUSE_FREETYPE=1                \
	-DUSE_LEVELDB=1                 \
	-DUSE_SQLITE=0                  \
	-DUSE_LUAJIT=1                  \
	-DUSE_GETTEXT=1                 \
	-D_IRR_COMPILE_WITH_SDL_DEVICE_ \
	-DVERSION_MAJOR=${versionMajor} \
	-DVERSION_MINOR=${versionMinor} \
	-DVERSION_PATCH=${versionPatch} \
	-DVERSION_EXTRA=${versionExtra} \
	-DDEVELOPMENT_BUILD=${developmentBuild} \
	$(GPROF_DEF)

ifdef NDEBUG
	LOCAL_CFLAGS += -DNDEBUG=1
endif

ifdef GPROF
	GPROF_DEF := -DGPROF
	PROFILER_LIBS := android-ndk-profiler
	LOCAL_CFLAGS += -pg
endif

LOCAL_C_INCLUDES := \
	../../../src                                    \
	../../../src/script                             \
	../../../lib/gmp                                \
	../../../lib/jsoncpp                            \
	deps/Android/Curl/include                       \
	deps/Android/Freetype/include                   \
	deps/Android/Irrlicht/include                   \
	deps/Android/libpng/include                     \
	deps/Android/libjpeg/include                    \
	deps/Android/SDL2/include                    \
	deps/Android/LevelDB/include                    \
	deps/Android/Gettext/include                    \
	deps/Android/LuaJIT/src                         \
	deps/Android/OpenAL-Soft/include                \
	deps/Android/Vorbis/include

LOCAL_SRC_FILES := \
	$(wildcard ../../../src/client/*.cpp)           \
	$(wildcard ../../../src/client/*/*.cpp)         \
	$(wildcard ../../../src/content/*.cpp)          \
	../../../src/database/database.cpp              \
	../../../src/database/database-dummy.cpp        \
	../../../src/database/database-files.cpp        \
	../../../src/database/database-leveldb.cpp      \
	$(wildcard ../../../src/gui/*.cpp)              \
	$(wildcard ../../../src/irrlicht_changes/*.cpp) \
	$(wildcard ../../../src/mapgen/*.cpp)           \
	$(wildcard ../../../src/network/*.cpp)          \
	$(wildcard ../../../src/script/*.cpp)           \
	$(wildcard ../../../src/script/common/*.cpp)    \
	$(wildcard ../../../src/script/cpp_api/*.cpp)   \
	../../../src/script/lua_api/l_areastore.cpp     \
	../../../src/script/lua_api/l_auth.cpp          \
	../../../src/script/lua_api/l_base.cpp          \
	../../../src/script/lua_api/l_camera.cpp        \
	../../../src/script/lua_api/l_craft.cpp         \
	../../../src/script/lua_api/l_client.cpp        \
	../../../src/script/lua_api/l_env.cpp           \
	../../../src/script/lua_api/l_http.cpp          \
	../../../src/script/lua_api/l_inventory.cpp     \
	../../../src/script/lua_api/l_item.cpp          \
	../../../src/script/lua_api/l_itemstackmeta.cpp \
	../../../src/script/lua_api/l_localplayer.cpp   \
	../../../src/script/lua_api/l_mainmenu.cpp      \
	../../../src/script/lua_api/l_mapgen.cpp        \
	../../../src/script/lua_api/l_metadata.cpp      \
	../../../src/script/lua_api/l_minimap.cpp       \
	../../../src/script/lua_api/l_modchannels.cpp   \
	../../../src/script/lua_api/l_nodemeta.cpp      \
	../../../src/script/lua_api/l_nodetimer.cpp     \
	../../../src/script/lua_api/l_noise.cpp         \
	../../../src/script/lua_api/l_object.cpp        \
	../../../src/script/lua_api/l_particles.cpp     \
	../../../src/script/lua_api/l_particles_local.cpp \
	../../../src/script/lua_api/l_playermeta.cpp    \
	../../../src/script/lua_api/l_server.cpp        \
	../../../src/script/lua_api/l_settings.cpp      \
	../../../src/script/lua_api/l_sound.cpp         \
	../../../src/script/lua_api/l_storage.cpp       \
	../../../src/script/lua_api/l_util.cpp          \
	../../../src/script/lua_api/l_vmanip.cpp        \
	$(wildcard ../../../src/server/*.cpp)           \
	$(wildcard ../../../src/threading/*.cpp)        \
	$(wildcard ../../../src/util/*.c)               \
	$(wildcard ../../../src/util/*.cpp)             \
	../../../src/ban.cpp                            \
	../../../src/chat.cpp                           \
	../../../src/clientiface.cpp                    \
	../../../src/collision.cpp                      \
	../../../src/content_mapnode.cpp                \
	../../../src/content_nodemeta.cpp               \
	../../../src/convert_json.cpp                   \
	../../../src/craftdef.cpp                       \
	../../../src/debug.cpp                          \
	../../../src/defaultsettings.cpp                \
	../../../src/emerge.cpp                         \
	../../../src/environment.cpp                    \
	../../../src/face_position_cache.cpp            \
	../../../src/filesys.cpp                        \
	../../../src/gettext.cpp                        \
	../../../src/httpfetch.cpp                      \
	../../../src/hud.cpp                            \
	../../../src/inventory.cpp                      \
	../../../src/inventorymanager.cpp               \
	../../../src/itemdef.cpp                        \
	../../../src/itemstackmetadata.cpp              \
	../../../src/light.cpp                          \
	../../../src/log.cpp                            \
	../../../src/main.cpp                           \
	../../../src/map.cpp                            \
	../../../src/map_settings_manager.cpp           \
	../../../src/mapblock.cpp                       \
	../../../src/mapnode.cpp                        \
	../../../src/mapsector.cpp                      \
	../../../src/metadata.cpp                       \
	../../../src/modchannels.cpp                    \
	../../../src/nameidmapping.cpp                  \
	../../../src/nodedef.cpp                        \
	../../../src/nodemetadata.cpp                   \
	../../../src/nodetimer.cpp                      \
	../../../src/noise.cpp                          \
	../../../src/objdef.cpp                         \
	../../../src/object_properties.cpp              \
	../../../src/particles.cpp                      \
	../../../src/pathfinder.cpp                     \
	../../../src/player.cpp                         \
	../../../src/porting.cpp                        \
	../../../src/porting_android.cpp                \
	../../../src/profiler.cpp                       \
	../../../src/raycast.cpp                        \
	../../../src/reflowscan.cpp                     \
	../../../src/remoteplayer.cpp                   \
	../../../src/serialization.cpp                  \
	../../../src/server.cpp                         \
	../../../src/serverenvironment.cpp              \
	../../../src/serverlist.cpp                     \
	../../../src/settings.cpp                       \
	../../../src/staticobject.cpp                   \
	../../../src/texture_override.cpp               \
	../../../src/tileanimation.cpp                  \
	../../../src/tool.cpp                           \
	../../../src/translation.cpp                    \
	../../../src/version.cpp                        \
	../../../src/voxel.cpp                          \
	../../../src/voxelalgorithms.cpp


# GMP
LOCAL_SRC_FILES += ../../../lib/gmp/mini-gmp.c

# JSONCPP
LOCAL_SRC_FILES += ../../../lib/jsoncpp/jsoncpp.cpp

# Lua UTF-8 Lib
LOCAL_SRC_FILES += ../../../lib/luautf8/lutf8lib.c

# Lua ChaCha Lib
LOCAL_SRC_FILES += $(wildcard ../../../lib/luachacha/*.c)

LOCAL_STATIC_LIBRARIES += \
	Curl mbedTLS mbedx509 mbedcrypto \
	Freetype \
	OpenAL \
	Gettext \
	Irrlicht libpng libjpeg SDL2 \
	LevelDB \
	Vorbis \
	LuaJIT

LOCAL_STATIC_LIBRARIES += $(PROFILER_LIBS)

LOCAL_LDLIBS := -lEGL -lGLESv1_CM -lGLESv2 -landroid -lOpenSLES -lz -llog

include $(BUILD_SHARED_LIBRARY)

ifdef GPROF
$(call import-module,android-ndk-profiler)
endif

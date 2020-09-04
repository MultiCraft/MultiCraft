APP_PLATFORM := ${APP_PLATFORM}
APP_ABI := ${TARGET_ABI}
APP_STL := c++_static
NDK_TOOLCHAIN_VERSION := clang
APP_SHORT_COMMANDS := true
APP_MODULES := MultiCraft

APP_CPPFLAGS := -Ofast -fvisibility=hidden -Wno-extra-tokens

ifeq ($(APP_ABI),armeabi-v7a)
APP_CPPFLAGS += -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb
endif

#ifeq ($(APP_ABI),x86)
#APP_CPPFLAGS += -march=i686 -mtune=intel -mssse3 -mfpmath=sse -m32 -funroll-loops
#endif

ifndef NDEBUG
APP_CPPFLAGS := -g -D_DEBUG -O0 -fno-omit-frame-pointer
endif

APP_CPPFLAGS += -fexceptions #-Werror=shorten-64-to-32

# Silence Irrlicht warnings. Comment out with real debugging!
APP_CPPFLAGS += -Wno-deprecated-declarations -Wno-inconsistent-missing-override

APP_CFLAGS   := $(APP_CPPFLAGS)
APP_CXXFLAGS := $(APP_CPPFLAGS) -frtti -std=gnu++17

ifdef NDEBUG
APP_LDFLAGS  := -Wl,--gc-sections,--icf=all
else
APP_LDFLAGS  :=
endif

APP_LDFLAGS  += -fuse-ld=lld

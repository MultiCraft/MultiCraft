APP_PLATFORM := ${APP_PLATFORM}
APP_ABI := ${TARGET_ABI}
APP_STL := c++_static
APP_SHORT_COMMANDS := true
APP_MODULES := MultiCraft

ifdef NDEBUG
APP_CFLAGS := -Ofast -fvisibility=hidden -Wno-extra-tokens -D__FILE__=__FILE_NAME__ -Wno-builtin-macro-redefined
else
APP_CFLAGS := -g -D_DEBUG -O0 -fno-omit-frame-pointer
endif

APP_CFLAGS += -fexceptions

APP_CXXFLAGS := $(APP_CFLAGS) -frtti -std=gnu++17 #-Werror=shorten-64-to-32

# Silence Irrlicht warnings. Comment out with real debugging!
APP_CXXFLAGS += -Wno-deprecated-declarations -Wno-inconsistent-missing-override

APP_CPPFLAGS := $(APP_CXXFLAGS)

ifdef NDEBUG
APP_LDFLAGS := -Wl,--gc-sections,--icf=all
endif

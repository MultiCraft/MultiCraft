add_library(luautf8 STATIC lutf8lib.c)
target_include_directories(luautf8 PUBLIC ${LUA_INCLUDE_DIR})

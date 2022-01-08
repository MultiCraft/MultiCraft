add_library(luachacha STATIC chacha.c lchacha.c)
target_include_directories(luachacha PUBLIC ${LUA_INCLUDE_DIR})

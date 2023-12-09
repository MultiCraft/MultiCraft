add_library(luachacha STATIC lchacha.c)
target_include_directories(luachacha PUBLIC ${LUA_INCLUDE_DIR} ${CHACHA_INCLUDE_DIR})
target_link_libraries(luachacha chacha)
add_definitions(-DSB_CONFIG_UNITY)
include_directories("Headers")
set(SHEENBIDI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Headers PARENT_SCOPE)
add_library(sheenbidi STATIC Source/SheenBidi.c)

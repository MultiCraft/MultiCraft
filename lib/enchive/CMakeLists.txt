add_library(enchive STATIC chacha.cpp sha256.cpp)
set(ENCHIVE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

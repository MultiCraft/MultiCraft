#!/bin/bash -e

# This file sets the appropriate compiler and flags for compiling for macOS
export OSX_OSVER=10.15
export MACOSX_DEPLOYMENT_TARGET=$OSX_OSVER

export OSX_ARCHES="x86_64 arm64"
export OSX_ARCHITECTURES="x86_64;arm64"
export OSX_ARCH="-arch x86_64 -arch arm64"

export CC=$(xcrun --sdk macosx --find clang)
export CXX=$(xcrun --sdk macosx --find clang++)
export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)

export OSX_FLAGS="-fdata-sections -ffunction-sections -fvisibility=hidden -fvisibility-inlines-hidden -O3 -ffast-math -flto -D__FILE__=__FILE_NAME__ -Wno-builtin-macro-redefined"

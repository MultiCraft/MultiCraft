#!/bin/bash -e

# This file sets the appropriate compiler and flags for compiling for macOS
sdk=macosx
export OSX_OSVER=10.11

export OSX_ARCHES="x86_64 arm64"
export OSX_ARCHITECTURES="x86_64;arm64"
export OSX_ARCH="-arch x86_64 -arch arm64"

export OSX_COMPILER=$(xcrun --sdk $sdk --find clang)
export OSX_CC=$OSX_COMPILER
export OSX_CXX=$OSX_COMPILER
export OSX_FLAGS="-isysroot $(xcrun --sdk $sdk --show-sdk-path) -mmacosx-version-min=$OSX_OSVER -fdata-sections -ffunction-sections -Ofast"

#!/bin/bash -e

# This file sets the appropriate compiler and flags for compiling for macOS
export OSX_OSVER=10.15
export MACOSX_DEPLOYMENT_TARGET=10.15

export OSX_ARCHES="x86_64 arm64"
export OSX_ARCHITECTURES="x86_64;arm64"
export OSX_ARCH="-arch x86_64 -arch arm64"

export OSX_CC=$(xcrun --sdk macosx --find clang)
export OSX_CXX=$(xcrun --sdk macosx --find clang++)
export OSX_FLAGS="-isysroot $(xcrun --sdk macosx --show-sdk-path) -mmacosx-version-min=$OSX_OSVER -fdata-sections -ffunction-sections -O3 -ffast-math"

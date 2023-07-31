#!/bin/bash -e

export CC=clang
export CXX=clang++

export CFLAGS="-fvisibility=hidden -fexceptions -O3"
export CXXFLAGS="$CFLAGS -frtti -O3"

export NPROC=$(( $(nproc) + 1 ))

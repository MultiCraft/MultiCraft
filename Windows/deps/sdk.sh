#!/bin/bash -e

export CFLAGS="-fvisibility=hidden -fexceptions -O3"
export CXXFLAGS="$CFLAGS -frtti -O3"

export NPROC=`(nproc) + 1`

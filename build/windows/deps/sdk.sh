#!/bin/bash -e

export CFLAGS="-fvisibility=hidden -fexceptions"
export CXXFLAGS="$CFLAGS -frtti"

export ANDR_ROOT=$(pwd)
export OUTPUT_PATH="$ANDR_ROOT/output"

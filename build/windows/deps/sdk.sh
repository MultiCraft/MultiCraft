#!/bin/bash -e

export CFLAGS="-fvisibility=hidden -fexceptions"
export CXXFLAGS="$CFLAGS -frtti"

#! /bin/bash -e

mkdir cmakebuild
cd cmakebuild
cmake -DCMAKE_BUILD_TYPE=Debug \
	-DUSE_SDL=ON \
	-DIRRLICHT_SOURCE_DIR="../deps/Irrlicht" \
	-DRUN_IN_PLACE=TRUE -DENABLE_GETTEXT=TRUE \
	-DBUILD_UNITTESTS=TRUE \
	-DUSE_ZSTD=TRUE \
	-DBUILD_SERVER=TRUE ${CMAKE_FLAGS} ..
make -j2

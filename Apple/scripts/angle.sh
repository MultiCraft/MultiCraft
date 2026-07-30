#!/bin/bash -e

ANGLE_VERSION=gles3-0.0.8

. scripts/sdk.sh
mkdir -p deps; cd deps

[ ! -d angle-src ] && \
	git clone --depth 1 -b $ANGLE_VERSION https://github.com/kakashidinho/metalangle.git angle-src

cd angle-src

# commit_id.py is invoked as "python"; provide it if only python3 exists
PYSHIM="$PWD/.pyshim"
if ! python --version >/dev/null 2>&1; then
	mkdir -p "$PYSHIM"
	printf '#!/bin/sh\nexec %s "$@"\n' "$(command -v python3)" > "$PYSHIM/python"
	chmod +x "$PYSHIM/python"
	export PATH="$PYSHIM:$PATH"
fi

./ios/xcode/fetchDependencies.sh

cd ios/xcode

xcodebuild build \
	-project OpenGLES.xcodeproj \
	-target MetalANGLE_mac \
	-sdk macosx \
	-configuration Release \
	ARCHS="$OSX_ARCHES" \
	ONLY_ACTIVE_ARCH=NO \
	MACOSX_DEPLOYMENT_TARGET=$OSX_OSVER \
	CODE_SIGNING_REQUIRED=NO \
	CODE_SIGNING_ALLOWED=NO \
	GCC_OPTIMIZATION_LEVEL=3 \
	LLVM_LTO=YES \
	OTHER_CFLAGS='$(inherited) -ffast-math -fno-finite-math-only -fdata-sections -ffunction-sections' \
	OTHER_CPLUSPLUSFLAGS='$(inherited) -ffast-math -fno-finite-math-only -fdata-sections -ffunction-sections'

cd ../..

rm -rf ../angle
mkdir -p ../angle
cp -R ios/xcode/build/Release/MetalANGLE.framework ../angle/
cp -R include ../angle/include

echo "ANGLE build successful"

#!/bin/bash

# Build engine

cd $TRAVIS_BUILD_DIR
mkdir -p osx-build && cd osx-build
export CFLAGS="-m32"
export CXXFLAGS="-m32"
cmake \
	-DXASH_SDL=yes \
	-DXASH_VGUI=yes \
	-DXASH_DLL_LOADER=no \
	-DXASH_DEDICATED=no \
	-DHL_SDK_DIR=../vgui-dev/ \
	-DCMAKE_OSX_ARCHITECTURES=i386 \
	-DMAINUI_USE_STB=yes \
	-DXASH_AUTODETECT_SSE_BUILD=no \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo ../
make -j2
cp engine/libxash.dylib mainui/libxashmenu.dylib vgui_support/libvgui_support.dylib ../vgui-dev/lib/vgui.dylib game_launch/xash3d ../xash3d.sh .
cp ~/Library/Frameworks/SDL2.framework/SDL2 libSDL2.dylib
tar -cjf $TRAVIS_BUILD_DIR/xash3d-osx.tar.bz2 libxash.dylib libxashmenu.dylib libvgui_support.dylib xash3d libSDL2.dylib xash3d.sh

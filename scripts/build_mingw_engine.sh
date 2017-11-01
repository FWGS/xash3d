#!/bin/bash

# Build engine

cd $TRAVIS_BUILD_DIR
mkdir -p mingw-build && cd mingw-build
export CC="ccache i686-w64-mingw32-gcc"
export CXX="ccache i686-w64-mingw32-g++"
export CFLAGS="-static-libgcc -no-pthread"
export CXXFLAGS="-static-libgcc -static-libstdc++"
cmake \
	-DSDL2_PATH=../sdl2-mingw/i686-w64-mingw32 \
	-DCMAKE_SYSTEM_NAME=Windows \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_PREFIX_PATH=../sdl2-mingw/i686-w64-mingw32 \
	-DXASH_STATIC=ON \
	-DXASH_VGUI=OFF \
	-DXASH_SDL=ON \
	-DXASH_AUTODETECT_SSE_BUILD=OFF ../
make -j2
cp engine/xash_sdl.exe mainui/menu.dll $TRAVIS_BUILD_DIR/vgui_support_bin/vgui_support.dll $TRAVIS_BUILD_DIR/sdl2-mingw/i686-w64-mingw32/bin/SDL2.dll .
cp /usr/i686-w64-mingw32/lib/libwinpthread-1.dll . # a1ba: remove when travis will be updated to xenial
7z a -t7z $TRAVIS_BUILD_DIR/xash3d-mingw.7z -m0=lzma2 -mx=9 -mfb=64 -md=32m -ms=on xash_sdl.exe menu.dll SDL2.dll vgui_support.dll libwinpthread-1.dll

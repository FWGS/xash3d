#!/bin/bash

# Build engine

cd $TRAVIS_BUILD_DIR
mkdir -p mingw-build && cd mingw-build
CC="ccache i686-w64-mingw32-gcc" CXX="ccache i686-w64-mingw32-g++" CFLAGS="-static-libgcc -no-pthread" CXXFLAGS="-static-libgcc -static-libstdc++ -no-pthread" LDFLAGS="-no-pthread" cmake -DSDL2_PATH=../sdl2-mingw/i686-w64-mingw32 -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_PREFIX_PATH=../sdl2-mingw/i686-w64-mingw32 -DXASH_STATIC=ON -DXASH_VGUI=no -DXASH_SDL=yes ../
make -j2
cp engine/xash_sdl.exe mainui/menu.dll $TRAVIS_BUILD_DIR/vgui_support_bin/vgui_support.dll $TRAVIS_BUILD_DIR/sdl2-mingw/i686-w64-mingw32/bin/SDL2.dll .
7z a -t7z $TRAVIS_BUILD_DIR/xash3d-mingw.7z -m0=lzma2 -mx=9 -mfb=64 -md=32m -ms=on xash_sdl.exe menu.dll SDL2.dll vgui_support.dll

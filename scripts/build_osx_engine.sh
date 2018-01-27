#!/bin/bash

# Build engine

cd $TRAVIS_BUILD_DIR
mkdir -p osx-build && cd osx-build
export CFLAGS="-m32"
export CXXFLAGS="-m32"
cmake \
	-DXASH_DOWNLOAD_DEPENDENCIES=yes \
	-DXASH_SDL=yes \
	-DXASH_VGUI=yes \
	-DXASH_DLL_LOADER=no \
	-DXASH_DEDICATED=no \
	-DCMAKE_OSX_ARCHITECTURES=i386 \
	-DMAINUI_USE_STB=yes \
	-DXASH_VECTORIZE_SINCOS=yes \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DLIB_INSTALL_DIR=\"\" \
	-DLIB_INSTALL_SUBDIR=\"\" \
	-DXASH_NO_INSTALL_RUNSCRIPT=yes ../
make -j2 VERBOSE=1
mkdir -p pkg/
make install DESTDIR=pkg/
cp ~/Library/Frameworks/SDL2.framework/SDL2 pkg/libSDL2.dylib
cp ../xash3d.sh pkg/
tar -cjf $TRAVIS_BUILD_DIR/xash3d-osx.tar.bz2 pkg/*

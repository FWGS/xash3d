#!/bin/bash

# Build custom SDL2

cd $TRAVIS_BUILD_DIR/SDL2-2.0.7
export CC="ccache gcc -msse2 -march=i686 -m32 -ggdb -O2"
./configure \
	--disable-dependency-tracking \
	--enable-audio \
	--enable-video \
	--enable-events \
	--disable-render \
	--enable-joystick \
	--disable-haptic \
	--disable-power \
	--enable-threads \
	--enable-timers \
	--enable-loadso \
	--enable-video-opengl \
	--enable-x11-shared \
	--enable-video-x11 \
	--enable-video-x11-xrandr \
	--enable-video-x11-scrnsaver \
	--enable-video-x11-xinput \
	--disable-video-x11-xshape \
	--disable-video-x11-xdbe \
	--disable-libudev \
	--disable-dbus \
	--disable-ibus \
	--enable-sdl-dlopen \
	--disable-video-opengles \
	--disable-cpuinfo \
	--disable-assembly \
	--disable-atomic \
	--enable-alsa
make -j2
mkdir -p $TRAVIS_BUILD_DIR/sdl2-linux
make install DESTDIR=$TRAVIS_BUILD_DIR/sdl2-linux

# Build engine

cd $TRAVIS_BUILD_DIR
mkdir -p build && cd build/
export CC="ccache gcc"
export CXX="ccache g++"
export CFLAGS="-m32"
export CXXFLAGS="-m32"
cmake \
	-DCMAKE_PREFIX_PATH=$TRAVIS_BUILD_DIR/sdl2-linux/usr/local \
	-DXASH_DOWNLOAD_DEPENDENCIES=yes \
	-DXASH_STATIC=ON \
	-DXASH_DLL_LOADER=ON \
	-DXASH_VGUI=ON \
	-DMAINUI_USE_STB=ON \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo ../
make -j2 VERBOSE=1
cp engine/xash3d mainui/libxashmenu.so vgui_support/libvgui_support.so vgui_support/vgui.so ../scripts/xash3d.sh .
cp $TRAVIS_BUILD_DIR/sdl2-linux/usr/local/lib/$(readlink $TRAVIS_BUILD_DIR/sdl2-linux/usr/local/lib/libSDL2-2.0.so.0) libSDL2-2.0.so.0
7z a -t7z $TRAVIS_BUILD_DIR/xash3d-linux.7z -m0=lzma2 -mx=9 -mfb=64 -md=32m -ms=on xash3d libSDL2-2.0.so.0 libvgui_support.so vgui.so libxashmenu.so xash3d.sh

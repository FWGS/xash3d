#!/bin/bash

cd $TRAVIS_BUILD_DIR/xash3d-android-project
sh gen-version.sh travis build
sh gen-config.sh test
python2 makepak.py xash-extras assets/extras.pak
ndk-build -j2 APP_CFLAGS="-w" NDK_CCACHE=ccache
ant debug -Dtest.version=1
cp bin/xashdroid-debug.apk $TRAVIS_BUILD_DIR/xash3d-generic.apk
tar cJf $TRAVIS_BUILD_DIR/android-debug-symbols.txz obj/local/*/*.so

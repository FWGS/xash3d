#!/bin/sh

jdk_switcher use oraclejdk8
curl -s http://dl.google.com/android/android-sdk_r22.0.4-linux.tgz | tar xzf -
export ANDROID_HOME=$PWD/android-sdk-linux
export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/platform-tools:$PWD/android-ndk
sleep 3s; echo y | android update sdk -u --filter platform-tools,build-tools-19.0.0,android-19 --force --all > /dev/null
wget http://dl.google.com/android/ndk/android-ndk-r10e-linux-x86_64.bin >/dev/null 2>/dev/null
7z x ./android-ndk-r10e-linux-x86_64.bin > /dev/null
mv android-ndk-r10e android-ndk
curl -s http://libsdl.org/release/SDL2-devel-2.0.7-mingw.tar.gz | tar xzf -
mv SDL2-2.0.7 sdl2-mingw
curl -s http://libsdl.org/release/SDL2-2.0.7.tar.gz | tar xzf -
git clone --depth 1 https://github.com/FWGS/xash3d-android-project

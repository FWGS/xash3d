#!/bin/sh

sudo apt-get -qq update >/dev/null
# a1ba: remove mingw-w64-i686-dev later, when travis will be updated to xenial, which have mingw with disabled pthread dependency
sudo apt-get -qq install --force-yes mingw-w64-i686-dev binutils-mingw-w64-i686 gcc-mingw-w64-i686 g++-mingw-w64-i686 p7zip-full gcc-multilib g++-multilib libx11-dev:i386 libxext-dev:i386 x11-utils libgl1-mesa-dev libasound-dev zlib1g:i386 libstdc++6:i386 >/dev/null
jdk_switcher use oraclejdk8
curl -s http://dl.google.com/android/android-sdk_r22.0.4-linux.tgz | tar xzf -
export ANDROID_HOME=$PWD/android-sdk-linux
export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/platform-tools:$PWD/android-ndk
sleep 3s; echo y | android update sdk -u --filter platform-tools,build-tools-19.0.0,android-19 --force --all > /dev/null
wget http://dl.google.com/android/ndk/android-ndk-r10e-linux-x86_64.bin >/dev/null 2>/dev/null
7z x ./android-ndk-r10e-linux-x86_64.bin > /dev/null
mv android-ndk-r10e android-ndk
curl -s http://libsdl.org/release/SDL2-devel-2.0.6-mingw.tar.gz | tar xzf -
mv SDL2-2.0.6 sdl2-mingw
curl -s http://libsdl.org/release/SDL2-2.0.6.tar.gz | tar xzf -
git clone --depth 1 https://github.com/FWGS/xash3d-android-project

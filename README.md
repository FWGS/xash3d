# Xash3D Engine [![Build Status](https://travis-ci.org/SDLash3D/xash3d.svg)](https://travis-ci.org/SDLash3D/xash3d) [![Windows Build Status](https://ci.appveyor.com/api/projects/status/github/SDLash3D/xash3d?branch=master&svg=true)](https://ci.appveyor.com/project/a1batross/xash3d)
Latest version of the library is available at:
https://github.com/SDLash3D/xash3d

Orignal project: [Xash3D on moddb](http://www.moddb.com/engines/xash3d-engine)

Xash3D Engine is a custom Gold Source engine rewritten from scratch. Xash3D
is compatible with many of the Gold Source games and mods and should be
able to run almost any existing singleplayer Half-Life mod without a hitch.
The multiplayer part is not yet completed, multiplayer mods should work just
fine, but bear in mind that some features may not work at all or work not
exactly the way they do in Gold Source Engine.

# How to build on Linux with CMake

If you want to enable VGUI support, you need [Half-Life 1 SDK](https://github.com/SDLash3D/halflife)
to build vgui_support library.
If you are using non-x86 architecture, you may skip this

Clone it with git:

    git submodule init && git submodule update

Implying Half Life 1 SDK is cloned into `hlsdk` you should be able to
build Xash3D as:
    
    cd xash3d
    mkdir -p build
    cd build
    cmake -DHL_SDK_DIR=../hlsdk -DXASH_SDL=yes -DXASH_VGUI=yes -DCMAKE_OSX_ARCHITECTURES=i386 ../
    make

# How to run    

After a successful build, copy the next files to some other directory where you want to run Xash3D:

    cp engine/libxash.so game_launch/xash3d mainui/libxashmenu.so $HOME/Games/Xash3D

If you built it with XASH_VGUI, also copy vgui.so:

    cp /opt/halflife/linux/vgui.so $HOME/Games/Xash3D

Copy **valve** folder from Half-Life:

    cp -r $HOME/.steam/steam/SteamApps/common/Half-Life/valve $HOME/Games/Xash3D
    
Copy script:

    cp ../xash3d.sh $HOME/Games/Xash3D

Run:

    $HOME/Games/Xash3D/xash3d.sh

# Building on linux manually without CMake

## Building engine

If you want to build Xash3D for some embedded device or just do not want to use CMake, use Makefile.linux instead

    cd (xash3d)/engine
    make -f Makefile.linux XASH_VGUI=1 XASH_SDL=1

or

    make -f Makefile.linux XASH_DEDICATED=1

To enable dll support on linux, build loader library:

    cd (xash3d)/loader
    make -f Makefile.linux libloader.so libloader.a
    cp libloader.so libloader.a ../engine/

And built engine with XASH_DLL_LOADER=1:

    make -f Makefile.linux XASH_VGUI=1 XASH_SDL=1 XASH_DLL_LOADER=1

or

    make -f Makefile.linux XASH_DEDICATED=1 XASH_DLL_LOADER=1

## After building engine, build launch binary:

    cd (xash3d)/game_launch
    gcc xash.c -o xash -ldl -lrt -lm

You may build engine with XASH_SINGLE_BINARY instead:

    make -f Makefile.linux XASH_SINGLE_BINARY=1 xash

## Running

Put xash3d binaries and vgui(optionally) to you game data directory and run:

    LD_LIBRARY_PATH=. ./xash -dev 5

### Running under GDB:

    LD_LIBRARY_PATH=. gdb --args ./xash -dev 5

### Using DLL Loader:

Put vgui_support.dll from windows build to your game data folder and run:

    LD_LIBRARY_PATH=. ./xash -dev 5 -vguilib vgui.dll -clientlib valve/cl_dlls/client.dll -dll dlls/hl.dll

# Building and running on Windows with Visual Studio 2013

Download latest prebuilt SDL2 from

https://www.libsdl.org/release/SDL2-devel-2.0.4-VC.zip

Unzip and rename `SDL2-2.0.4` folder to `SDL2` and put it next to xash3d project folder.

    ..\xash3d\
    ..\SDL2\
    
Open `xash.sln` with Visual Studio 2013 and make a build. After building, copy contents of `Debug` or `Release` folder to directory you choose. Copy `valve` folder and `vgui.dll` from your Half Life game installation folder and `SDL2.dll` form `\SDL2\lib\x86\` to it. Move `vgui_support.dll` into `valve` folder.

    ..\valve\
    ..\valve\vgui_support.dll
    ..\menu.dll
    ..\SDL2.dll
    ..\vgui.dll
    ..\xash.dll
    ..\xash.exe

Now you good to go, just run `xash.exe`.

# Building with Visual Studio 6

This is legacy configuration, but msvc6 seems to generate more stable and more effective code

Setup your msvc6 enviroment, unpack SDL2-2.0.4 to xash3d folder and do:

    cd (xash3d)\engine
    ..\msvc6\build.bat
    cd (xash3d)\game_launch
    ..\msvc6\build_launch.bat

# Building with MinGW:

The most effective configuration, but maybe unstable.
Setup your MinGW environment and run:

    cd (xash3d)\engine\
    mingw32-make -f Makefile.mingw

    engine will be built to single exe binary

# License

The library is licensed under GPLv3 license, see COPYING for details.
CMakeLists.txt files are licensed under MIT license, you will find it's text
in every CMakeLists.txt header.

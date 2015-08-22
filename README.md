# Xash3D Engine [![Build Status](https://travis-ci.org/SDLash3D/xash3d.svg)](https://travis-ci.org/SDLash3D/xash3d) [![Build Status](https://ci.appveyor.com/api/projects/status/github/MrYadro/xash3d?branch=master&svg=true)](https://travis-ci.org/SDLash3D/xash3d)
Latest version of the library is available at:
https://github.com/SDLash3D/xash3d

Orignal project: [Xash3D on moddb](http://www.moddb.com/engines/xash3d-engine)

Xash3D Engine is a custom Gold Source engine rewritten from scratch. Xash3D
is compatible with many of the Gold Source games and mods and should be
able to run almost any existing singleplayer Half-Life mod without a hitch.
The multiplayer part is not yet completed, multiplayer mods should work just
fine, but bear in mind that some features may not work at all or work not
exactly the way they do in Gold Source Engine.

# How to build

Xash3D Engine requires [Half-Life 1 SDK](https://github.com/SDLash3D/halflife).
Clone it with git:

    git clone https://github.com/SDLash3D/halflife

Implying Half Life 1 SDK is cloned into /opt/halflife you should be able to
build Xash3D as:
    
    cd xash3d
    mkdir -p build
    cd build
    cmake -DHL_SDK_DIR=/opt/halflife -DXASH_SDL=yes -DXASH_VGUI=yes -DCMAKE_OSX_ARCHITECTURES=i386 ..
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

# License

The library is licensed under GPLv3 license, see COPYING for details.
CMakeLists.txt files are licensed under MIT license, you will find it's text
in every CMakeLists.txt header.

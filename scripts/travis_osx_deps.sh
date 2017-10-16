#!/bin/sh

curl -s https://www.libsdl.org/release/SDL2-2.0.6.dmg > SDL2-2.0.6.dmg
hdiutil attach SDL2-2.0.6.dmg
cd /Volumes/SDL2
mkdir -p ~/Library/Frameworks
cp -r SDL2.framework ~/Library/Frameworks/

exit 0

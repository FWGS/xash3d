# Emscripten support

## Emscripten patches

### Unaligned access

Currently emscripten builds needs patched emscripten to work because some function performs unaligned memory loads and stores.

Patches adding workaround for unaligned access:

* https://github.com/FWGS/emscripten-fastcomp/commit/8a8aabaf12787c766ef330ca54091177fa97dbca
* https://github.com/FWGS/emscripten-fastcomp/commit/ae80eea58451cea21f3d58362747ebffa3a58d20
* https://github.com/FWGS/emscripten/commit/5ab1f691454a2aa86ab08456600c9180fc00a907

### Texture binding

Xash3D uses old-style texture creation which does not work with Emscripten

Patch to make texture loading work:

* https://github.com/FWGS/emscripten/commit/f42bf7d9e3e8ec9f6c67349e12a102a413d7ff9d

### Network proxy

Originally, emsctipten uses raw websockets to implement network protocols.
This means that browser client will be compatible only with emscripten-compiled server (with NodeJS for example),
but separate server newer will have enough players to play

Better solution is make proxy that supports both TCP and UDP connections

Proxy server side (still dirty, but works):

https://github.com/mittorn/websockify-c/tree/udp

Emscripten patch for wsproxy protocol:
* https://github.com/FWGS/emscripten/commit/efcb8ecd0807c5590637812a29b4d1c7cd582719

set proxy server url in xash.html or shell.html

### ...

All patches compiled in this branches:

* https://github.com/FWGS/emscripten/tree/xash3d
* https://github.com/FWGS/emscripten-fastcomp/tree/master

* x86_64 linux build: https://github.com/FWGS/emscripten-fastcomp/releases/tag/xash3d-0.1

## Compiling

### Preparing emscripten

To build xash3d for emscripten, clone forked emscripten to home directory

```
git clone https://github.com/FWGS/emscripten -b xash3d
wget https://github.com/FWGS/emscripten-fastcomp/releases/download/xash3d-0.1/fastcomp.txz -O - |tar xJ
```

Set correct emscripten path in ~/.emscripten to make emcc work

### Preparing xash3d repo

clone xash3d repo:

```
git clone https://github.com/FWGS/xash3d
cd xash3d
git submodule init && git submodule update
```

Setup makefiles:

```
./contrib/mittorn/setup.sh
```

Switch to branch containing this file

Clone nanogl and gl-wes-2 repos:

```
cd engine
git clone https://github.com/FWGS/gl-wes-v2 -b webgl-vbo
git clone https://github.com/FWGS/nanogl
```

Edit Makefile.emscripten to ensure using correct output paths (by default, ~/xash-em and ~/emscripten used)

`make -f Makefile.emscripten -j5`

### Building game libraries

Clone microndk repo

`git clone https://github.com/FWGS/microndk`

Change dir to mainui_cpp, dlls or cl_dlls directory and do:

`make -f /path/to/microndk/Microndk.mk CC=/patch/to/emcc CXX=/path/to/em++`

You may use unpatched emscripten to build game libraries if mod does not have unaligned access

But half-life saverestore code has:
* https://github.com/FWGS/hlsdk-xash3d/blob/9ebfc981773ec4c7a89ffe52d9c249e1fbef9634/dlls/util.cpp#L2137

This may be fixed by replacing all access to pInputData and pOutputData by memcpy calls

## Game data

Use mods.js to define paths to game data on server (see engine/platform/emscripten/mods.js.example)

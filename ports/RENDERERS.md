# OpenGL translators support

## NanoGL

### Android

Enabled by default in xash3d-android-project

### iOS

dependency of xash3d-ios project

### Linux

```
cd xash3d
./contrib/mittorn/setup.sh
cd engine
git clone https://github.com/FWGS/nanogl
make -f Makefile.linux NANOGL=1
```

### Windows

Support is undone. Partial support may be enabled in Makefile.mingw or build_nanogl.bat

## gl-wes-v2

### Linux

```
cd xash3d
./contrib/mittorn/setup.sh
cd engine
git clone https://github.com/FWGS/gl-wes-v2
git clone https://github.com/FWGS/nanogl
make -f Makefile.linux WES=1
```

### Emscrepten

look add README.emscripten.md

## regal

### Linux

Support is very poor because regal does not support GL1 enough
```
cd xash3d
./contrib/mittorn/setup.sh
cd engine
git clone https://github.com/p3/regal --depth 1
git clone https://github.com/FWGS/nanogl
```

Configure and build regal for your target platform

Build xash3d with this options:

`make -f Makefile.linux REGAL=1`
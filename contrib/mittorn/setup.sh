#!/bin/sh
SCRIPTDIR=$(dirname "$0")
TOPDIR=$SCRIPTDIR/../../
cp Makefile.dllloader $TOPDIR/loader/
cp Makefile.emscripten $TOPDIR/engine/
cp Makefile.linux $TOPDIR/engine/
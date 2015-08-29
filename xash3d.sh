#!/bin/bash

SCRIPT_DIR=${PWD##*/}

if [ "$SCRIPT_DIR" == "bin" ]; then
# Xash3D SDL is installed in system, so run it from lib/xash3d/ directory with Steam HL basedir if XASH3D_BASEDIR is not set
	if [ -z "$XASH3D_BASEDIR" ]; then
		export XASH3D_BASEDIR="$HOME/.steam/steam/steamapps/common/Half-Life/"
	fi
	GAMEROOT=${PWD}/../lib/xash3d
	echo "Xash3D SDL is installed in system. Running from ${PWD}/../lib/xash3d/"
else
	GAMEROOT=$(dirname -- $(readlink -f -- $0))
fi

#determine platform
UNAME=`uname`
if [ "$UNAME" == "Darwin" ]; then
	# prepend our lib path to LD_LIBRARY_PATH
	export DYLD_LIBRARY_PATH=${GAMEROOT}:$DYLD_LIBRARY_PATH
elif [ "$UNAME" == "Linux" ]; then
	# prepend our lib path to LD_LIBRARY_PATH
	export LD_LIBRARY_PATH=${GAMEROOT}:$LD_LIBRARY_PATH
fi

if [ -z $GAMEEXE ]; then
	if [ "$UNAME" == "Darwin" ]; then
		GAMEEXE=xash3d
	elif [ "$UNAME" == "Linux" ]; then
		GAMEEXE=xash3d
	fi
fi

ulimit -n 2048

# and launch the game
cd "$GAMEROOT"

STATUS=42
while [ $STATUS -eq 42 ]; do
	${DEBUGGER} "${GAMEROOT}"/${GAMEEXE} $@
	STATUS=$?
done
exit $STATUS

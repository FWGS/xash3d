#!/bin/sh

SCRIPT_DIR=${PWD##*/}
IS64BIT=$(getconf LONG_BIT)

if [ "$IS64BIT" = "64" ]; then
	LIBPREFIX="lib32"
else
	LIBPREFIX="lib"
fi

if [ "$SCRIPT_DIR" = "bin" ]; then
# Xash3D SDL is installed in system, so run it from lib/xash3d/ directory with Steam HL basedir if XASH3D_BASEDIR is not set
	if [ -z "$XASH3D_BASEDIR" ]; then
		export XASH3D_BASEDIR="$HOME/.steam/steam/steamapps/common/Half-Life/"
	fi
	GAMEROOT=${PWD}/../${LIBPREFIX}/xash3d
	echo "Xash3D SDL is installed in system."
else
	GAMEROOT=$(dirname -- "$(readlink -f -- "$0")")
fi

#determine platform
UNAME=$(uname)
if [ "$UNAME" = "Darwin" ]; then
	# prepend our lib path to LD_LIBRARY_PATH
	export DYLD_LIBRARY_PATH=${GAMEROOT}:$DYLD_LIBRARY_PATH
elif [ "$UNAME" = "Linux" ]; then
	# prepend our lib path to LD_LIBRARY_PATH
	export LD_LIBRARY_PATH=${GAMEROOT}:$LD_LIBRARY_PATH
elif [ "$UNAME" = "FreeBSD" ]; then
	export LD_LIBRARY_PATH=${GAMEROOT}:$LD_LIBRARY_PATH
fi

if [ -z "$GAMEEXE" ]; then
	if [ "$UNAME" = "Darwin" ]; then
		GAMEEXE=xash3d
	elif [ "$UNAME" = "Linux" ]; then
		GAMEEXE=xash3d
	elif [ "$UNAME" = "FreeBSD" ]; then
		GAMEEXE=xash3d
	fi
fi

# and launch the game
cd "$GAMEROOT" || echo "Failed cd to $GAMEROOT" && exit

STATUS=42
while [ $STATUS -eq 42 ]; do
	${DEBUGGER} "${GAMEROOT}"/${GAMEEXE} "$@"
	STATUS=$?
done
exit $STATUS

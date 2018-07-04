#!/bin/sh

SCRIPT_DIR=${PWD##*/}
SCRIPT_NAME=$(basename "$0")
IS64BIT=$(getconf LONG_BIT)

if [ "$IS64BIT" = "64" ]; then
	LIBPREFIX="lib32"
else
	LIBPREFIX="lib"
fi

if [ -z "$GAMEEXE" ]; then
	GAMEEXE=${SCRIPT_NAME%.*} # strip extension(not required, but do anyway)
fi

if [ "$SCRIPT_DIR" = "bin" ]; then
	# Xash3D SDL is installed in system, so run it from lib/xash3d/ directory with Steam HL basedir if XASH3D_BASEDIR is not set
	if [ -d "$XASH3D_BASEDIR" ]; then
		export XASH3D_BASEDIR="$HOME/.steam/steam/steamapps/common/Half-Life/"
	fi
	GAMEROOT=${PWD}/../${LIBPREFIX}/${GAMEEXE}
	echo "Xash3D FWGS is installed in system."
else
	GAMEROOT=$(dirname -- "$(readlink -f -- "$0")")
fi

#determine platform
UNAME=$(uname)
if [ "$UNAME" = "Darwin" ]; then
	# prepend our lib path to DYLD_LIBRARY_PATH
	export DYLD_LIBRARY_PATH=${GAMEROOT}:$DYLD_LIBRARY_PATH
else
	# prepend our lib path to LD_LIBRARY_PATH
	export LD_LIBRARY_PATH=${GAMEROOT}:$LD_LIBRARY_PATH
fi

# and launch the game
if ! cd "$GAMEROOT"; then
	echo "Failed cd to $GAMEROOT"
	exit
fi

STATUS=42
while [ $STATUS -eq 42 ]; do
	${DEBUGGER} "${GAMEROOT}"/"${GAMEEXE}" "$@"
	STATUS=$?
done
exit $STATUS

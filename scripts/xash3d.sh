#!/bin/sh

SCRIPT_DIR=$(cd "$(dirname -- "$0")" && pwd)
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

# Check if Xash3D FWGS installed in system
if [ "$SCRIPT_DIR" = "bin" ]; then
	# Run it from lib/xash3d/ directory with Steam HL basedir if XASH3D_BASEDIR is not set
	if [ -z "$XASH3D_BASEDIR" ]; then
		if [ -d "$HOME/.steam/steam/steamapps/common/Half-Life/" ]; then
			export XASH3D_BASEDIR="$HOME/.steam/steam/steamapps/common/Half-Life/"
		else
			export XASH3D_BASEDIR="$PWD"
		fi
	fi
	GAMEROOT=${PWD}/../${LIBPREFIX}/${GAMEEXE}
	echo "Xash3D FWGS is installed in system. Game directory is set to: $XASH3D_BASEDIR"
else
	GAMEROOT="$SCRIPT_DIR"
fi

#determine platform
UNAME=$(uname)
if [ "$UNAME" = "Darwin" ]; then
	# prepend our lib path to DYLD_LIBRARY_PATH
	export DYLD_LIBRARY_PATH="${GAMEROOT}:$DYLD_LIBRARY_PATH"
else
	# prepend our lib path to LD_LIBRARY_PATH
	export LD_LIBRARY_PATH="${GAMEROOT}:$LD_LIBRARY_PATH"
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

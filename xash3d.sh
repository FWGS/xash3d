#!/bin/bash

GAMEROOT=$(dirname -- $(readlink -f -- $0))

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

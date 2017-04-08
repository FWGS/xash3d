#
# Copyright (c) 2015 Pavlo Lavrenenko
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 2.6.0)
project (LOADER)

set (LOADER_LIBRARY loader)

file (GLOB_RECURSE LOADER_SOURCES *.c)
include_directories(. wine/ ../engine/common ../engine ../engine/client ../common )
if(XASH_STATIC)
	add_library (${LOADER_LIBRARY} STATIC ${LOADER_SOURCES})
else()
	add_library (${LOADER_LIBRARY} SHARED ${LOADER_SOURCES})
endif()

IF (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "NetBSD" OR CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
       target_link_libraries (${LOADER_LIBRARY} -lm -pthread -lexecinfo)
ELSE ()
	   target_link_libraries (${LOADER_LIBRARY} -lm -pthread)
ENDIF ()

set_target_properties (${LOADER_SHARED} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

install( TARGETS ${LOADER_LIBRARY} DESTINATION ${LIB_INSTALL_DIR}/xash3d 
	        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

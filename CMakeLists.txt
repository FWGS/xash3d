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
project (XASH3D)

#--------------
# USER DEFINES \
################\

set (XASH3D_VERSION 0.15)
set (HL_SDK_DIR /opt/halflife CACHE PATH "Half Life 1 SDK directory")

option(XASH_DEDICATED "Enable dedicated build." ${XASH_DEDICATED})
option(XASH_DLL_LOADER "Enable DLL loading on x86 Linux." ${XASH_DLL_LOADER})
option(XASH_IPX "Enable IPX support for networking. Windows only." ${XASH_IPX})
option(XASH_GLES "Enable fixes if Xash3D is running over GL to GLES translator." ${XASH_GLES})
option(XASH_NONSTANDARD_LOAD "Enable loading game libraries. Windows only." ${XASH_NONSTANDARD_LOAD})
option(XASH_SDL "Enable SDL for client." ${XASH_SDL})
option(XASH_VGUI "Enable VGUI support." ${XASH_VGUI})
option(XASH_X11 "Enable X11 support." ${XASH_VGUI})

#-----------------
# MAIN BUILD CODE \
###################\

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Build 32-bit Xash on 64-bit, because Xash3D not support this
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	set (CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	if(NOT DEFINED LIB_SUFFIX) 
		set (LIB_SUFFIX "32")
	endif()
endif()

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
endif()
if(NOT DEFINED LIBEXEC_INSTALL_DIR)
    set(LIBEXEC_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/libexec")
endif()
if(NOT DEFINED BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
endif()

if(NOT XASH_DEDICATED)
	add_subdirectory (mainui)
endif()
if(XASH_DLL_LOADER)
	add_subdirectory (loader)
endif()
if(XASH_VGUI)
	add_subdirectory (vgui_support)
endif()
add_subdirectory (engine)
add_subdirectory (game_launch)

install(FILES "xash3d.sh"
    DESTINATION ${BIN_INSTALL_DIR}
    RENAME "xash3d"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

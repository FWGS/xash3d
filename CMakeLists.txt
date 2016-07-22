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

set (XASH3D_VERSION 0.18)

set (HL_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hlsdk/)

# Servers must choose this
option(XASH_DEDICATED "Enable dedicated build. Servers must choose this." OFF)

# Windows only. Enable it by default for Windows.
if(NOT WIN32)
	option(XASH_IPX "Enable IPX support for networking. Windows only." OFF)
	option(XASH_NONSTANDARD_LOAD "Enable loading game libraries. Windows only." OFF)
else()
	option(XASH_IPX "Enable IPX support for networking. Windows only." ON)
	option(XASH_NONSTANDARD_LOAD "Enable loading game libraries. Windows only." ON)
endif()

# GLES hacks
option(XASH_GLES "Enable fixes if Xash3D is running over GL to GLES translator." OFF)
option(XASH_NANOGL "Enable NanoGL. Implicitly enables XASH_GLES." OFF)

# For libdl-haters ;)
option(XASH_SINGLE_BINARY "Don't build game launcher and build engine as executaable" OFF)
option(XASH_STATIC "Static build. Implicitly enables XASH_SINGLE_BINARY." OFF)

# Shared library loader implementation. Must be checked one of these or both.
option(XASH_LIBDL "Enable libdl" ON)
option(XASH_DLL_LOADER "Enable DLL loading on x86 Linux/*nix." ON)

# Desktop-version options
option(XASH_SDL "Enable SDL." ON)
option(XASH_VGUI "Enable VGUI support." ON)
option(XASH_X11 "Enable X11 support." ON)

#-----------------
# MAIN BUILD CODE \
###################\

# Dependencies
if(XASH_STATIC)
	set(XASH_SINGLE_BINARY "ON")
endif()

if(XASH_NANOGL)
	set(XASH_GLES "ON")
endif()

if(NOT XASH_LIBDL AND NOT XASH_DLL_LOADER)
	message(FATAL_ERROR "You must choose XASH_LIBDL, XASH_DLL_LOADER or both. Otherwise engine is useless.")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Build 32-bit Xash on 64-bit, because Xash3D not support this
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
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
	if(XASH_VGUI)
		add_subdirectory (vgui_support)
	endif()
endif()

if(XASH_DLL_LOADER)
	add_subdirectory (loader)
endif()

if(NOT XASH_SINGLE_BINARY)
	add_subdirectory (game_launch)
endif()

add_subdirectory (engine)

install(FILES "xash3d.sh"
    DESTINATION ${BIN_INSTALL_DIR}
    RENAME "xash3d"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

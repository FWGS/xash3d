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

set (XASH3D_VERSION 0.19.1)

set (HL_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hlsdk/)

include( CMakeDependentOption )

# Servers must choose this
option(XASH_DEDICATED "Enable dedicated build. Servers must choose this." OFF)

# Windows only. Enable it by default for Windows.
cmake_dependent_option(XASH_IPX "Enable IPX support for networking. Windows only." ON "WIN32" OFF)
cmake_dependent_option(XASH_NONSTANDARD_LOAD "Enable own DLL loader in Windows." ON "WIN32" OFF)

# POSIX only. Disable it by default for Linux
cmake_dependent_option(XASH_USE_SELECT "Enable reading console commands from stdin." ON "NOT WIN32" OFF)

# GLES hacks
option(XASH_GLES "Enable fixes if Xash3D is running over GL to GLES translator." OFF)
option(XASH_NANOGL "Enable NanoGL. Implicitly enables XASH_GLES." OFF)

# For libdl-haters ;)
option(XASH_SINGLE_BINARY "Don't build game launcher and build engine as executaable" OFF)
option(XASH_STATIC "Static build. Implicitly enables XASH_SINGLE_BINARY." OFF)

# Shared library loader implementation. Must be checked one of these or both.
cmake_dependent_option(XASH_LIBDL "Enable libdl use. POSIX only." ON "NOT WIN32" OFF)
option(XASH_DLL_LOADER "Enable DLL loading on x86 Linux/*nix." OFF)

# Enable 64-bit build of Xash3D FWGS Engine.
option(XASH_64BIT "Enable experimental 64-bit build. Note that you must recompile game for 64 bit." OFF)

# Desktop-version options
if(XASH_DEDICATED) 
	option(XASH_SDL "Enable SDL." OFF) # Dedicated servers still can use SDL
	set(XASH_VGUI OFF) # But these dedicated can't use at all, so hardcode it as disabled
	set(XASH_X11 OFF)
else()
	option(XASH_SDL "Enable SDL." ON)
	option(XASH_VGUI "Enable VGUI support." ON)
	if( NOT WIN32 )
		option(XASH_X11 "Enable X11 support." ON)
	else()
		option(XASH_X11 "Enable X11 support." OFF)
	endif()
endif()

# Pre-build analyze step
option(XASH_PVS_ANALYZE "Use PVS-Studio static analyzer" NO)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

#-----------------
# MAIN BUILD CODE \
###################\

if(NOT XASH_64BIT AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(FATAL_ERROR "Xash3D doesn't work with 64 bit systems well. Please, set CMake to build for 32 system. If you still want 64bit build, pass -DXASH_64BIT=1 to CMake")
endif()

# Dependencies
if(XASH_STATIC)
	set(XASH_SINGLE_BINARY "ON")
endif()

if(XASH_NANOGL)
	set(XASH_GLES "ON")
endif()

if(NOT XASH_LIBDL AND NOT XASH_DLL_LOADER AND NOT XASH_NONSTANDARD_LOAD)
	message(FATAL_ERROR "You must choose XASH_LIBDL, XASH_DLL_LOADER or both. Otherwise engine is useless.")
endif()


if(NOT DEFINED LIB_INSTALL_DIR)
	if( WIN32 )
		set( LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_BUILD_TYPE}" )
	else()
		set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
	endif()
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

if(NOT WIN32)
	install(FILES "xash3d.sh"
	    DESTINATION ${BIN_INSTALL_DIR}
	    RENAME "xash3d"
	    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 	GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

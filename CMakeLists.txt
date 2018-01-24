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

cmake_minimum_required(VERSION 3.0.0)

# Install custom module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

include(VSForceXPToolchain) # Force XP toolchain for Visual Studio

project (XASH3D)

include(FWGSLib)
include(CMakeDependentOption)
include(CPack)

#--------------
# USER DEFINES \
################\

set (XASH3D_VERSION 0.19.1)

option(XASH_DEDICATED "Enable dedicated build. Servers must choose this." OFF) # Servers must choose this

# Windows only. Enable it by default for Windows.
option(XASH_IPX "Enable IPX support for networking. Windows only. Deprecated." OFF)
cmake_dependent_option(XASH_NONSTANDARD_LOAD "Enable own DLL loader in Windows." ON "WIN32" OFF)

# POSIX only. Disable it by default for Windows
cmake_dependent_option(XASH_USE_SELECT "Enable reading console commands from stdin." ON "NOT WIN32" OFF)
cmake_dependent_option(XASH_LIBDL "Enable libdl use. POSIX only." ON "NOT WIN32" OFF)
option(XASH_DLL_LOADER "Enable DLL loading on x86 Linux/*nix." OFF)

option(XASH_GLES "Enable fixes if Xash3D is running over GL to GLES translator." OFF)
option(XASH_NANOGL "Enable NanoGL. Implicitly enables XASH_GLES." OFF)
option(XASH_STATIC "Static build. Implicitly enables XASH_SINGLE_BINARY." OFF)
option(XASH_NO_ASYNC_NS_RESOLVE "Disable asynchronous domain name resolving." OFF)

# Enable 64-bit build of Xash3D FWGS Engine.
option(XASH_64BIT "Enable experimental 64-bit build. Note that you must also provide 64-bit game binaries" OFF)
cmake_dependent_option(XASH_VECTORIZE_SINCOS "Use vectorized versions of sin(), cos() and sincos()" ON "XASH_64BIT" OFF)

# Desktop or dedicated version options
if(XASH_DEDICATED) 
	option(XASH_SDL "Enable SDL." OFF) # Dedicated servers still can use SDL
	set(XASH_SINGLE_BINARY ON) # Disable game launcher by default for dedicated
	set(XASH_VGUI OFF) # But these dedicated can't use at all, so hardcode it as disabled
	set(XASH_MAINUI OFF) # But these dedicated can't use at all, so hardcode it as disabled
else()
	option(XASH_SDL "Enable SDL." ON)
	option(XASH_SINGLE_BINARY "Don't build game launcher and build engine as executable" OFF)
	option(XASH_VGUI "Enable VGUI support." ON)
	option(XASH_MAINUI "Build mainui library" ON) # Totally not recommended to disable
endif()

# Misc
option(XASH_NO_INSTALL_RUNSCRIPT "Don't install xash3d.sh" OFF)
option(XASH_NO_INSTALL_VGUI_BIN "Don't bundle proprietary VGUI" OFF)
if(WIN32) # Autodownload for lazy developers using Windows
	option(XASH_DOWNLOAD_DEPENDENCIES "Download dependencies automatically, if supported" ON)
else()
	option(XASH_DOWNLOAD_DEPENDENCIES "Download dependencies automatically, if supported" OFF)
endif()

# Installing
if(WIN32)
	fwgs_string_option(LIB_INSTALL_DIR "Libraries install directory" ${CMAKE_BINARY_DIR})
	fwgs_string_option(LIB_INSTALL_SUBDIR "Libraries install subdirectory" "")
	fwgs_string_option(BIN_INSTALL_DIR "Executables install directory" ${CMAKE_BINARY_DIR})
else()
	fwgs_string_option(LIB_INSTALL_DIR "Libraries install directory" "lib")
	fwgs_string_option(LIB_INSTALL_SUBDIR "Libraries install subdirectory" "xash3d")
	fwgs_string_option(BIN_INSTALL_DIR "Executables install directory" "bin")
endif()


# /*
# ==============
# xash_link_sdl2
#
# Download and link SDL2, if supported
# ==============
# */
macro(xash_link_sdl2 tgt)
	if(WIN32)
		set(SDL2_VER "2.0.7")
		set(SDL2_RUNTIME "VC")
		set(SDL2_SUBDIR "SDL2-${SDL2_VER}")
		if(MINGW)
			if(XASH_64BIT)
				set(SDL2_SUBDIR "SDL2-${SDL2_VER}/x86_64-w64-mingw32")
			else()
				set(SDL2_SUBDIR "SDL2-${SDL2_VER}/i686-w64-mingw32")
			endif()
			set(SDL2_RUNTIME "mingw")
		endif()

		set(SDL2_DOWNLOAD_URL "http://libsdl.org/release/SDL2-devel-${SDL2_VER}-${SDL2_RUNTIME}.zip")
		set(SDL2_SUBDIR "SDL2-${SDL2_VER}")

		fwgs_library_dependency(${tgt} SDL2 ${SDL2_DOWNLOAD_URL} "SDL2.zip" SDL2_PATH ${SDL2_SUBDIR})
	else()
		# SDL2 doesn't provide dev packages for nonWin32 targets
		fwgs_library_dependency(${tgt} SDL2)
	endif()
endmacro()

if(NOT XASH_64BIT AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(FATAL_ERROR "You're building 64 bit Xash3D! Note that there is almost no games compiled for 64-bit. Please, set CMake to build for 32 system. If you still want 64bit build, pass -DXASH_64BIT=1 to CMake")
endif()

if(NOT XASH_LIBDL AND NOT XASH_DLL_LOADER AND NOT XASH_NONSTANDARD_LOAD)
	message(FATAL_ERROR "You must enable XASH_LIBDL, XASH_DLL_LOADER or both. Otherwise engine is useless.")
endif()

# Dependencies
if(XASH_STATIC)
	set(XASH_SINGLE_BINARY ON)
endif()

if(XASH_NANOGL)
	set(XASH_GLES ON)
endif()

fwgs_conditional_subproject(XASH_MAINUI mainui)
fwgs_conditional_subproject(XASH_VGUI vgui_support)
fwgs_conditional_subproject(XASH_DLL_LOADER loader)
fwgs_conditional_subproject("NOT XASH_SINGLE_BINARY" game_launch)
add_subdirectory(engine)

if(NOT WIN32 OR XASH_NO_INSTALL_RUNSCRIPT)
	install(FILES "scripts/xash3d.sh"
		DESTINATION "${BIN_INSTALL_DIR}"
	    RENAME "xash3d"
		PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

if(WIN32)
	if(XASH_SINGLE_BINARY)
		set_property(DIRECTORY . PROPERTY VS_STARTUP_PROJECT xash) # Engine project
	else()
		set_property(DIRECTORY . PROPERTY VS_STARTUP_PROJECT xash3d) # Game_launch project
	endif()
endif()

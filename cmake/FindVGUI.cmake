#
# Copyright (c) 2017 Flying With Gauss
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

include(FindPackageHandleStandardArgs)

if(NOT HL_SDK_DIR AND NOT XASH_DOWNLOAD_DEPENDENCIES)
	message( FATAL_ERROR "Pass a HL_SDK_DIR variable to CMake to be able use VGUI" )
endif()

set(VGUI_SEARCH_PATHS ${HL_SDK_DIR})

find_path(VGUI_INCLUDE_DIR 
	VGUI.h
	HINTS $ENV{VGUIDIR}
	PATH_SUFFIXES
	    utils/vgui/include/   # HLSDK
		include/              # vgui-dev
	PATHS ${VGUI_SEARCH_PATHS}
)

message(STATUS "<FindVGUI.cmake> ${VGUI_SEARCH_PATHS}")

if(APPLE)
	set(LIBNAMES vgui.dylib)
else()
	set(LIBNAMES vgui vgui.so)
endif()

find_library(VGUI_LIBRARY
	NAMES ${LIBNAMES}
	HINTS $ENV{VGUIDIR}
	PATH_SUFFIXES 
		games/lib/xash3d         # libvgui debian package
		lib/xash3d               # Maybe installed in system?
		xash3d/			 # Maybe installed in system? [2]
		utils/vgui/lib/win32_vc6 # Win32 VC6
		linux/                   # Linux
		linux/release            # OSX
		lib/win32_vc6			 # vgui-dev Win32 VC6
		lib/                     # vgui-dev Linux & OSX
	PATHS ${VGUI_SEARCH_PATHS}
)

# HACKHACK: as you can see, other compilers and OSes 
# can easily link to vgui library, no matter how it was placed
# On Linux just target_link_libraries will give you a wrong
# binary, which have ABSOLUTE PATH to vgui.so!
# Stupid Linux linkers just check for a path and this may give
# a TWO SAME libraries in memory, which obviously goes to crash engine

# EXAMPLE(without hack):
# $ LD_LIBRARY_PATH=$(pwd) ldd libvgui_support.so
#       /home/user/projects/hlsdk/linux/vgui.so => /home/user/projects/hlsdk/linux/vgui.so (addr)
# With hack:
# $ LD_LIBRARY_PATH=$(pwd) ldd libvgui_support.so
#       vgui.so => $(pwd)/vgui.so

macro(target_link_vgui_hack arg1)
	if(WIN32)
		target_link_libraries(${arg1} ${VGUI_LIBRARY} )
	elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		target_link_libraries(${arg1} ${VGUI_LIBRARY} )
	elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		add_custom_command(TARGET ${arg1} PRE_LINK COMMAND
			${CMAKE_COMMAND} -E copy ${VGUI_LIBRARY} $<TARGET_FILE_DIR:${arg1}>)
		target_link_libraries(${arg1} -L. -L${arg1}/ -l:vgui.so)
	endif()
endmacro()

find_package_handle_standard_args(VGUI REQUIRED_VARS VGUI_LIBRARY VGUI_INCLUDE_DIR)

message(STATUS "</FindVGUI.cmake> ${VGUI_LIBRARY}")


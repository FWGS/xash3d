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
project (VGUI_SUPPORT)

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set (CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -O3")

set (VGUI_SUPPORT_LIBRARY vgui_support)

file (GLOB VGUI_SUPPORT_SOURCES *.cpp *.c)
include_directories (. ../common ../engine ../engine/common ../engine/client ../engine/client/vgui ${HL_SDK_DIR}/utils/vgui/include/)

add_library (${VGUI_SUPPORT_LIBRARY} SHARED ${VGUI_SUPPORT_SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(${VGUI_SUPPORT_LIBRARY} -L${HL_SDK_DIR}/linux/release/ ${HL_SDK_DIR}/linux/release/vgui.dylib)
endif()
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    #Make GCC don't put full path to vgui.so
    add_custom_command(TARGET vgui_support PRE_LINK COMMAND
	${CMAKE_COMMAND} -E copy ${HL_SDK_DIR}/linux/vgui.so $<TARGET_SONAME_FILE_DIR:vgui_support>)
    target_link_libraries(${VGUI_SUPPORT_LIBRARY} -L. -l:vgui.so)
endif()

add_definitions(-DXASH_VGUI)

set_target_properties (${VGUI_SUPPORT_SHARED} PROPERTIES
	VERSION ${XASH3D_VERSION} SOVERSION ${XASH3D_VERSION}
	POSITION_INDEPENDENT_CODE 1)

install( TARGETS ${VGUI_SUPPORT_LIBRARY} DESTINATION ${LIB_INSTALL_DIR}/xash3d
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ )
install( FILES $<TARGET_SONAME_FILE_DIR:vgui_support>/vgui.so DESTINATION ${LIB_INSTALL_DIR}/xash3d
	        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ )

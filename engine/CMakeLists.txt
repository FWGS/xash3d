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
project (XASH_ENGINE)

include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2_IMAGE REQUIRED SDL2_image)

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set (CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -O3")

if(PANDORA)
        add_definitions(-DPANDORA)
endif()

set (XASH_ENGINE_LIBRARY xash)

file (GLOB_RECURSE XASH_ENGINE_SOURCES *.cpp *.c)
file (GLOB_RECURSE XASH_ENGINE_HEADERS *.h)
include_directories (. common/
    common/imagelib/
    common/soundlib/
    client/
    client/vgui/
    server/
    ../common
    ../pm_shared
    ${HL_SDK_DIR}/public
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    )

add_library (${XASH_ENGINE_LIBRARY} SHARED ${XASH_ENGINE_HEADERS} ${XASH_ENGINE_SOURCES})

if(XASH_X11)
    add_definitions(-DXASH_X11)
    target_link_libraries(${XASH_ENGINE_LIBRARY} X11)
endif()

if(XASH_IPX)
    add_definitions(-DXASH_IPX)
endif()

if(XASH_NONSTANDART_LOAD)
    add_definitions(-DXASH_NONSTANDART_LOAD)
endif()

if(XASH_VGUI)
    add_definitions(-DXASH_VGUI)
    target_link_libraries(${XASH_ENGINE_LIBRARY} -l:${HL_SDK_DIR}/linux/vgui.so)
    INCLUDE_DIRECTORIES(${HL_SDK_DIR}/utils/vgui/include)
endif()

if(XASH_GLES)
    add_definitions(-DXASH_GLES)
endif()

target_link_libraries(${XASH_ENGINE_LIBRARY}
    ${CMAKE_DL_LIBS} ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})

set_target_properties (${XASH_ENGINE_SHARED} PROPERTIES
    VERSION ${XASH3D_VERSION} SOVERSION ${XASH3D_VERSION}
    POSITION_INDEPENDENT_CODE 1)

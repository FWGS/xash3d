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

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set (CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -O3")

set (CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
set (CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
set (CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

set (XASH_ENGINE_LIBRARY xash)

file (GLOB_RECURSE XASH_ENGINE_SOURCES *.cpp *.c)
file (GLOB_RECURSE XASH_ENGINE_HEADERS *.h)
include_directories (. common/
    common/sdl/
    common/imagelib/
    common/soundlib/
    client/
    client/vgui/
    server/
    ../common
    ../pm_shared
    ${HL_SDK_DIR}/public
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

if(XASH_SDL)
    find_package(SDL2 REQUIRED)

    include_directories(${SDL2_INCLUDE_DIR})

    add_definitions(-DXASH_SDL)
    target_link_libraries(${XASH_ENGINE_LIBRARY} ${SDL2_LIBRARY})
endif()

if(XASH_VGUI)
    add_definitions(-DXASH_VGUI)
#    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#        target_link_libraries(${XASH_ENGINE_LIBRARY} -L${HL_SDK_DIR}/linux/release/ ${HL_SDK_DIR}/linux/release/vgui.dylib)
#    endif()
#    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#        target_link_libraries(${XASH_ENGINE_LIBRARY} -L${HL_SDK_DIR}/linux/ -l:vgui.so)
#    endif()
#    INCLUDE_DIRECTORIES(${HL_SDK_DIR}/utils/vgui/include)
endif()

if(XASH_GLES)
    add_definitions(-DXASH_GLES)
endif()

if(XASH_DLL_LOADER)
    add_definitions(-DDLL_LOADER)

    # Make GCC don't put full path to libloader.so
    add_custom_command(TARGET xash PRE_LINK COMMAND
	${CMAKE_COMMAND} -E copy $<TARGET_FILE:loader> $<TARGET_SONAME_FILE_DIR:xash>)
    target_link_libraries(${XASH_ENGINE_LIBRARY} -L. -lloader)
endif()

if(XASH_DEDICATED)
    add_definitions(-DXASH_DEDICATED)
    if(LINUX)
    	target_link_libraries(${XASH_ENGINE_LIBRARY} -lrt)
    endif()
endif()

target_link_libraries(${XASH_ENGINE_LIBRARY}
    ${CMAKE_DL_LIBS})

set_target_properties (${XASH_ENGINE_SHARED} PROPERTIES
    VERSION ${XASH3D_VERSION} SOVERSION ${XASH3D_VERSION}
    POSITION_INDEPENDENT_CODE 1)

# Installing
install( TARGETS ${XASH_ENGINE_LIBRARY} DESTINATION ${LIB_INSTALL_DIR}/xash3d 
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

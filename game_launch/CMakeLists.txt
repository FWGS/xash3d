#
# Copyright (c) 2015 Pavlo Lavrenenko, Alibek Omarov
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
project (XASH_LAUNCHER)

if(XASH_SDL AND NOT WIN32) # WinMain is used
	find_package(SDL2 REQUIRED sdl2)
	include_directories(${SDL2_INCLUDE_DIR})

	add_definitions(-DXASH_SDL)
endif()

set (XASH_LAUNCHER_EXECUTABLE xash3d)

file (GLOB XASH_LAUNCHER_SOURCES *.c)
if(WIN32)
	list(APPEND XASH_LAUNCHER_SOURCES game.rc)
endif()

add_executable (${XASH_LAUNCHER_EXECUTABLE} WIN32 ${XASH_LAUNCHER_SOURCES})

if(XASH_SDL AND NOT WIN32) # WinMain is used
	target_link_libraries(${XASH_LAUNCHER_EXECUTABLE} ${SDL2_LIBRARY})
endif()

if(WIN32)
	set_target_properties(${XASH_LAUNCHER_EXECUTABLE} PROPERTIES
		OUTPUT_NAME xash)
endif()

target_link_libraries(${XASH_LAUNCHER_EXECUTABLE}
	${CMAKE_DL_LIBS})

if( NOT WIN32 )
	install( TARGETS ${XASH_LAUNCHER_EXECUTABLE} DESTINATION ${LIB_INSTALL_DIR}/xash3d )
else()
	install( TARGETS ${XASH_LAUNCHER_EXECUTABLE}
		CONFIGURATIONS Debug 
		RUNTIME DESTINATION ${LIB_INSTALL_DIR}/Debug/
		LIBRARY DESTINATION ${LIB_INSTALL_DIR}/Debug/ )
	install( FILES $<TARGET_FILE_DIR:${XASH_LAUNCHER_EXECUTABLE}>/$<TARGET_PROPERTY:${XASH_LAUNCHER_EXECUTABLE},OUTPUT_NAME>.pdb
		CONFIGURATIONS Debug 
		DESTINATION ${LIB_INSTALL_DIR}/Debug/ )
	install( TARGETS ${XASH_LAUNCHER_EXECUTABLE}
		CONFIGURATIONS Release 
		RUNTIME DESTINATION ${LIB_INSTALL_DIR}/Release/
		LIBRARY DESTINATION ${LIB_INSTALL_DIR}/Release/ )
endif()

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
project(VGUI_SUPPORT)

set(VGUI_SUPPORT_LIBRARY vgui_support)

file(GLOB VGUI_SUPPORT_SOURCES *.cpp *.c)
include_directories(
	.
	../common
	../engine
	../engine/common
	../engine/client
	../engine/client/vgui
)

add_library(${VGUI_SUPPORT_LIBRARY} SHARED ${VGUI_SUPPORT_SOURCES})

find_package(VGUI REQUIRED)

include_directories(${VGUI_INCLUDE_DIR})

target_link_vgui_hack(${VGUI_SUPPORT_LIBRARY})

add_definitions(-DXASH_VGUI)
set_target_properties(${VGUI_SUPPORT_LIBRARY} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

if(WIN32)
	set_target_properties(${VGUI_SUPPORT_LIBRARY} PROPERTIES
		PREFIX "")
endif()

if(NOT WIN32)
	install(TARGETS ${VGUI_SUPPORT_LIBRARY} DESTINATION ${LIB_INSTALL_DIR}/${LIB_INSTALL_SUBDIR}
		    PERMISSIONS OWNER_READ OWNER_WRITE
			            GROUP_READ
						WORLD_READ)
	install(FILES ${VGUI_LIBRARY} DESTINATION ${LIB_INSTALL_DIR}/${LIB_INSTALL_SUBDIR}
		    PERMISSIONS OWNER_READ OWNER_WRITE
			            GROUP_READ
						WORLD_READ)
else()
	install(TARGETS ${VGUI_SUPPORT_LIBRARY}
		CONFIGURATIONS Debug 
		RUNTIME DESTINATION ${LIB_INSTALL_DIR}/Debug/
		LIBRARY DESTINATION ${LIB_INSTALL_DIR}/Debug/)
	if(MSVC)
		install(FILES $<TARGET_PDB_FILE:${VGUI_SUPPORT_LIBRARY}>
			CONFIGURATIONS Debug 
			DESTINATION ${LIB_INSTALL_DIR}/Debug/)
	endif()
	install(TARGETS ${VGUI_SUPPORT_LIBRARY}
		CONFIGURATIONS Release 
		RUNTIME DESTINATION ${LIB_INSTALL_DIR}/Release/
		LIBRARY DESTINATION ${LIB_INSTALL_DIR}/Release/)
endif()

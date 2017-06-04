#
# Copyright (c) 2016 Alibek Omarov
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
cmake_minimum_required( VERSION 2.6.0 )
project( XASH_ENGINE C )

set( XASH_ENGINE_LIBRARY xash )	    # Lib name
file( GLOB_RECURSE XASH_ENGINE_SOURCES common/*.c server/*.c ) # Minimal sources
file( GLOB_RECURSE XASH_ENGINE_WIN32_SOURCES platform/win32/*.c ) # Win32 only sources
file( GLOB_RECURSE XASH_ENGINE_SDL_SOURCES platform/sdl/*.c ) # SDL version sources
file( GLOB_RECURSE XASH_ENGINE_ANDROID_SOURCES platform/android/*.c ) # Android sources
file( GLOB_RECURSE XASH_ENGINE_CLIENT_SOURCES client/*.c ) # Client sources

# TODO: How to evaluate expression in set?
if( NOT XASH_DEDICATED )
	set( _XASH_CLIENT 1 ) # Hack for a macro. Also just a shortcut
else( )
	set( _XASH_CLIENT 0 )
endif( )

macro( cond_list_append arg1 arg2 arg3 )
	if( ${arg1} )
		list( APPEND ${arg2} ${${arg3}} )
	endif( )
endmacro( cond_list_append )

cond_list_append( WIN32 XASH_ENGINE_SOURCES XASH_ENGINE_WIN32_SOURCES )
cond_list_append( XASH_SDL XASH_ENGINE_SOURCES XASH_ENGINE_SDL_SOURCES )
cond_list_append( ANDROID XASH_ENGINE_SOURCES XASH_ENGINE_ANDROID_SOURCES )
cond_list_append( _XASH_CLIENT XASH_ENGINE_SOURCES XASH_ENGINE_CLIENT_SOURCES )

include_directories ( . common/ common/imagelib/  common/soundlib/ client/ client/vgui/ server/ ../common ../pm_shared )

if( XASH_AUTODETECT_SSE_BUILD )
	include( CheckSSEFeatures )
	if( SSE_FLAGS )
		add_compile_options( ${SSE_FLAGS} )
		add_definitions( -DVECTORIZE_SINCOS )
	endif( )
endif()

add_definitions( -DXASH_FORCEINLINE )

# Set executable or library
if( XASH_SINGLE_BINARY )
	add_executable( ${XASH_ENGINE_LIBRARY} ${XASH_ENGINE_SOURCES} )
else( )
	add_library( ${XASH_ENGINE_LIBRARY} SHARED ${XASH_ENGINE_SOURCES} )
endif( )

# ----- Conditions -----

if( MSVC )
	add_definitions( -D_CRT_SECURE_NO_WARNINGS 
		-D_CRT_NONSTDC_NO_DEPRECATE 
		-DXASH_CRASHHANDLER=2 -DXASH_W32CON ) # dbghelp crashhandler
else()
	if( XASH_USE_SELECT )
		add_definitions( -DUSE_SELECT )
	endif()
	add_definitions( -DCOLORIZE_CONSOLE )
endif() 

if( LINUX )
	add_definitions( -DGDB_BREAK )
endif()

if( NOT XASH_64BIT )
	add_definitions( -DXASH_FASTSTR )
endif()

if( XASH_IPX )
	add_definitions( -DXASH_IPX )
endif( )

if( XASH_NONSTANDART_LOAD )
	add_definitions( -DXASH_NONSTANDART_LOAD )
endif( )

if( XASH_DEDICATED )
	add_definitions( -DXASH_DEDICATED )
	if( LINUX )
		target_link_libraries( ${XASH_ENGINE_LIBRARY} -lrt )
	endif( )
else( )
	if( XASH_X11 )
		add_definitions( -DXASH_X11 )
		target_link_libraries( ${XASH_ENGINE_LIBRARY} X11 )
	endif( )

	if( XASH_VGUI )
		add_definitions( -DXASH_VGUI )
	endif( )
endif( )

if( XASH_SDL )
	find_package( SDL2 REQUIRED )

	include_directories( ${SDL2_INCLUDE_DIR} )

	add_definitions( -DXASH_SDL )
	target_link_libraries( ${XASH_ENGINE_LIBRARY} ${SDL2_LIBRARY} )
endif( )

if( XASH_GLES )
	add_definitions( -DXASH_GLES )
endif( )

if( XASH_DLL_LOADER )
	add_definitions( -DDLL_LOADER )

	include_directories( ../loader )

    # Make GCC don't put full path to libloader.so
	add_custom_command( TARGET xash PRE_LINK COMMAND ${CMAKE_COMMAND} -E
		copy $<TARGET_FILE:loader> $<TARGET_FILE_DIR:xash> )

	target_link_libraries( ${XASH_ENGINE_LIBRARY} -L. -lloader )
endif( )

if( NOT XASH_LIBDL )
	add_definitions( -DNO_LIBDL )
else( )
	target_link_libraries( ${XASH_ENGINE_LIBRARY} ${CMAKE_DL_LIBS} )
endif( )

if( NOT XASH_RELEASE )
	execute_process( COMMAND "git" "rev-parse" "--short" "HEAD"
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		OUTPUT_VARIABLE SHORT_HASH
		OUTPUT_STRIP_TRAILING_WHITESPACE )

	if( SHORT_HASH )
		message( STATUS "git hash: ${SHORT_HASH}" )
		add_definitions( -DXASH_BUILD_COMMIT="${SHORT_HASH}" )
	else( )
		message( STATUS "git hash: not set" )
	endif( )
else( )
	add_definitions( -DXASH_RELEASE )
endif( )


if( NOT WIN32 )
	target_link_libraries( ${XASH_ENGINE_LIBRARY} -lm )
endif( )

set_target_properties( ${XASH_ENGINE_LIBRARY} PROPERTIES
	POSITION_INDEPENDENT_CODE 1 )

if( WIN32 )
	if( XASH_DEDICATED )
		set_target_properties( ${XASH_ENGINE_LIBRARY} PROPERTIES
			OUTPUT_NAME "xash_dedicated" )
	else()
		set_target_properties( ${XASH_ENGINE_LIBRARY} PROPERTIES
			OUTPUT_NAME "xash_sdl" )
	endif()
endif( )

# Installing
if( NOT WIN32 )
	install( TARGETS ${XASH_ENGINE_LIBRARY} DESTINATION ${LIB_INSTALL_DIR}/xash3d 
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
else()
	install( TARGETS ${XASH_ENGINE_LIBRARY}
		CONFIGURATIONS Debug 
		RUNTIME DESTINATION ${LIB_INSTALL_DIR}/Debug/
		LIBRARY DESTINATION ${LIB_INSTALL_DIR}/Debug/ )
	install( FILES $<TARGET_FILE_DIR:${XASH_ENGINE_LIBRARY}>/$<TARGET_PROPERTY:${XASH_ENGINE_LIBRARY},OUTPUT_NAME>.pdb
		CONFIGURATIONS Debug 
		DESTINATION ${LIB_INSTALL_DIR}/Debug/ )
	install( TARGETS ${XASH_ENGINE_LIBRARY}
		CONFIGURATIONS Release 
		RUNTIME DESTINATION ${LIB_INSTALL_DIR}/Release/
		LIBRARY DESTINATION ${LIB_INSTALL_DIR}/Release/ )
endif()

# Analyzer
if( XASH_PVS_ANALYZE )
	set( PVS_STUDIO_TARGET ${XASH_ENGINE_LIBRARY} )
	set( PVS_STUDIO_CONFIG ${CMAKE_CURRENT_LIST_DIR}/../PVS-Studio.cfg )
	#set(PVS_STUDIO_ANALYSIS_MODE 31) See PVS-Studio --help

	include( PVSAnalyze )
endif( )

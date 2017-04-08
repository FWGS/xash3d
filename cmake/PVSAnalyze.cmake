#
# Copyright (c) 2016 Alibek Omarov a.k.a. a1batross
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


# How to use
#
# PVS_STUDIO_TARGET and PVS_STUDIO_CONFIG must be set before including this script
# PVS_STUDIO_TARGET is equal your current target: library, executable, etc
# PVS_STUDIO_CONFIG is equal path to your config.
# PVS_STUDIO_ANALYSIS_MODE equals mask representing analysis mode. See PVS-Studio --help

message("<PVSAnalyze.cmake>")

find_program(PVS_STUDIO PVS-Studio)

if(NOT PVS_STUDIO)
	message(FATAL_ERROR "PVS-Studio was not found in your system!")
endif()

if(NOT PVS_STUDIO_TARGET)
	message(FATAL_ERROR "PVS_STUDIO_TRAGET variable not set. Set it to your target.")
endif()

if(NOT PVS_STUDIO_CONFIG)
	message(FATAL_ERROR "PVS_STUDIO_CONFIG not found. Please set up your config file: http://www.viva64.com/en/d/0519/")
endif()

if(NOT PVS_STUDIO_ANALYSIS_MODE)
	set(PVS_STUDIO_ANALYSIS_MODE 0)
endif()

if(${CMAKE_BUILD_TYPE} EQUAL "Debug")
	set(PVS_STUDIO_CONFIGURATION "Debug")
else()
	set(PVS_STUDIO_CONFIGURATION "Release")
endif()

# I'm unsure here.
# If binary only amd64, what --platform does?
#if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
#	set(PVS_STUDIO_PLATFORM "linux64")
#else()
#	set(PVS_STUDIO_PLATFORM "linux32")
#endif()

get_target_property(PVS_STUDIO_SOURCES ${PVS_STUDIO_TARGET} SOURCES)

if(NOT PVS_STUDIO_SOURCES)
	message(FATAL_ERROR "Failed to query source file list for ${PVS_STUDIO_TARGET}")
endif()

set(PVS_STUDIO_INCLUDE $<TARGET_PROPERTY:${PVS_STUDIO_TARGET},INCLUDE_DIRECTORIES>)
set(PVS_STUDIO_DEFINES $<TARGET_PROPERTY:${PVS_STUDIO_TARGET},COMPILE_DEFINITIONS>)
set(PVS_STUDIO_OPTIONS $<TARGET_PROPERTY:${PVS_STUDIO_TARGET},COMPILE_OPTIONS>)

# GCC/Clang Only!
set(PVS_STUDIO_COMPILER_INCLUDE "$<$<BOOL:${PVS_STUDIO_INCLUDE}>:-I$<JOIN:${PVS_STUDIO_INCLUDE}, -I>>")
set(PVS_STUDIO_COMPILER_DEFINES "$<$<BOOL:${PVS_STUDIO_DEFINES}>:-D$<JOIN:${PVS_STUDIO_DEFINES}, -D>>")

set(PVS_STUDIO_FLAGS "${CMAKE_C_FLAGS} ${PVS_STUDIO_OPTIONS} ${PVS_STUDIO_COMPILER_INCLUDE} ${PVS_STUDIO_COMPILER_DEFINES}")

foreach(PVS_STUDIO_SOURCE_FILE IN LISTS PVS_STUDIO_SOURCES)
	get_source_file_property(PVS_STUDIO_SOURCE_FILE_LOCATION ${PVS_STUDIO_SOURCE_FILE} LOCATION)

	add_custom_command(TARGET ${PVS_STUDIO_TARGET}
		PRE_BUILD
		COMMAND sh -c "${PVS_STUDIO} --cfg ${PVS_STUDIO_CONFIG} --configuration ${PVS_STUDIO_CONFIGURATION} --source-file ${PVS_STUDIO_SOURCE_FILE_LOCATION} --analysis-mode ${PVS_STUDIO_ANALYSIS_MODE} --cl-params ${PVS_STUDIO_FLAGS} ${PVS_STUDIO_SOURCE_FILE_LOCATION}"
		COMMENT "[----] Analyzing ${PVS_STUDIO_SOURCE_FILE_LOCATION} with PVS-Studio..."
		VERBATIM)
endforeach()

message("</PVSAnalyze.cmake>")

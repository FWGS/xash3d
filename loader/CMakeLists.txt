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

cmake_minimum_required(VERSION  2.8.0)
project(LOADER C)

set(LOADER_LIBRARY loader)

file(GLOB_RECURSE LOADER_SOURCES *.c)
include_directories(. wine/ ../engine/common ../engine ../engine/client ../common)

add_library(${LOADER_LIBRARY} STATIC ${LOADER_SOURCES})

if(XASH_SDL)
	xash_link_sdl2(${LOADER_LIBRARY})
	add_definitions(-DSDL)
endif()

fwgs_add_compile_options(C -mstackrealign)

if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") # Compiler-dependent options
	fwgs_add_compile_options(C -mstack-alignment=12)
else()
	fwgs_add_compile_options(C -mpreferred-stack-boundary=12)
endif()

target_link_libraries(${LOADER_LIBRARY} -lm -pthread)

if(CMAKE_SYSTEM_NAME MATCHES ".+BSD$")
	target_link_libraries (${LOADER_LIBRARY} -lexecinfo)
endif()

fwgs_set_default_properties(${LOADER_LIBRARY})
# Don't install, we are static library
# fwgs_install(${LOADER_LIBRARY})

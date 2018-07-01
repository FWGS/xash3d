#
# Copyright (c) 2018 a1batross
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

option(FWGSLIB_DEBUG "Print debug messages for FWGSLib" OFF)
option(XASH_FORCE_PIE "Force enable -fpie/-fPIC" OFF) # may be required by some systems, although not recommended for i386

# /*
# ================
# fwgs_debug
# ================
# */
macro(fwgs_debug)
	if(FWGSLIB_DEBUG)
		message(${ARGV})
	endif()
endmacro()

# /*
# ================
# fwgs_install
#
# Common installation function for FWGS projects
# When using VS it tries to simulate VS behaviour for default installing
# Otherwise *nix-style
# ================
# */
macro(fwgs_install tgt)
	if(NOT MSVC)
		install(TARGETS ${tgt} DESTINATION ${LIB_INSTALL_DIR}/${LIB_INSTALL_SUBDIR}
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
			    GROUP_READ GROUP_EXECUTE
				WORLD_READ WORLD_EXECUTE) # chmod 755
	else()
		install(TARGETS ${tgt}
			CONFIGURATIONS Debug
			RUNTIME DESTINATION ${LIB_INSTALL_DIR}/Debug/
			LIBRARY DESTINATION ${LIB_INSTALL_DIR}/Debug/)
		install(FILES $<TARGET_PDB_FILE:${tgt}>
			CONFIGURATIONS Debug
			DESTINATION ${LIB_INSTALL_DIR}/Debug/ )
		install(TARGETS ${tgt}
			CONFIGURATIONS Release
			RUNTIME DESTINATION ${LIB_INSTALL_DIR}/Release/
			LIBRARY DESTINATION ${LIB_INSTALL_DIR}/Release/)
	endif()
endmacro()

# /*
# ================
# fwgs_conditional_subproject
#
# Default properties for target
# ================
# */
macro(fwgs_conditional_subproject cond subproject)
	set(TEMP 1)
	foreach(d ${cond})
		string(REGEX REPLACE " +" ";" expr "${d}")
		if(${expr})
		else()
			set(TEMP 0)
		endif()
    endforeach()
	if(TEMP)
		add_subdirectory(${subproject})
	endif()
endmacro()

# /*
# ================
# fwgs_set_default_properties
#
# Default properties for target
# ================
# */
macro(fwgs_set_default_properties tgt)
	if( XASH_FORCE_PIE OR (APPLE AND CMAKE_OSX_ARCHITECTURES) OR NOT (ARCH STREQUAL "i386"))
		message(STATUS "Enabled PIE for ${tgt}")
		set_target_properties(${tgt} PROPERTIES
			POSITION_INDEPENDENT_CODE 1)
	else()
		message(STATUS "Disabled PIE for ${tgt}")
	endif()
	if(WIN32)
		set_target_properties(${tgt} PROPERTIES
			PREFIX "")
	endif()
endmacro()

# /*
# ===============
# fwgs_string_option
#
# like option(), but for string
# ===============
# */
macro(fwgs_string_option name description value)
	set(${name} ${value} CACHE STRING ${description})
endmacro()

# /*
# ================
# cond_list_append
#
# Append to list by condition check
# ================
# */
macro(cond_list_append arg1 arg2 arg3)
	set(TEMP 1)
	foreach(d ${arg1})
		string(REGEX REPLACE " +" ";" expr "${d}")
		if(${expr})
		else()
			set(TEMP 0)
		endif()
    endforeach()

	if(TEMP)
		list(APPEND ${arg2} ${${arg3}})
	endif()
endmacro(cond_list_append)

macro(fwgs_unpack_file file path)
	message(STATUS "Unpacking ${file} to ${CMAKE_BINARY_DIR}/${path}")
	execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/${path})
	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${file}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${path})
endmacro()

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

# /*
# ================
# fwgs_link_package
#
# Downloads(if enabled), finds and links library
# ================
# */
macro(fwgs_library_dependency tgt pkgname)
	if(XASH_DOWNLOAD_DEPENDENCIES AND ${ARGC} GREATER 2)
		set(FORCE_DOWNLOAD FALSE)
		set(FORCE_UNPACK FALSE)

		# Disabled, due to bugs
		# find_package(${pkgname}) # First try to find it in system!
		if(NOT ${${pkgname}_FOUND}) # Not found anything, download it
			set(FORCE_DOWNLOAD TRUE)
			set(FORCE_UNPACK TRUE)
		else()
			# I see what you did there, CMake Cache!
			if(NOT EXISTS ${${pkgname}_LIBRARY})
				# Check if we unpacked
				if(NOT EXISTS ${CMAKE_BINARY_DIR}/${pkgname})
					set(FORCE_UNPACK TRUE)

					# Check if we need re-download
					if(NOT EXISTS "${CMAKE_BINARY_DIR}/${ARGV3}")
						set(FORCE_DOWNLOAD TRUE)
					endif()
				endif()
			endif()
		endif()

		if(FORCE_DOWNLOAD)
			# Download
			message(STATUS "Downloading ${pkgname} dependency for ${tgt} from ${ARGV2} to ${ARGV3}")
			file(DOWNLOAD "${ARGV2}" "${CMAKE_BINARY_DIR}/${ARGV3}")
			set(FORCE_UNPACK TRUE) # Update directory contents
		endif()

		if(FORCE_UNPACK)
			fwgs_unpack_file("${CMAKE_BINARY_DIR}/${ARGV3}" "${pkgname}")
		endif()

		if(FORCE_DOWNLOAD OR FORCE_UNPACK)
			set(${ARGV4} ${CMAKE_BINARY_DIR}/${pkgname}/${ARGV5}) # Pass subdirectory, like VGUI/vgui-dev-master or SDL2/SDL2-2.0.x
			find_package(${pkgname} REQUIRED) # Now try it hard
		endif()
	else()
		find_package(${pkgname} REQUIRED) # Find in system
	endif()

	include_directories(${${pkgname}_INCLUDE_DIR})
	if(${pkgname} STREQUAL VGUI) #HACKHACK: VGUI link
		target_link_vgui_hack(${tgt})
	else()
		target_link_libraries(${tgt} ${${pkgname}_LIBRARY})
	endif()
endmacro()

# /*
# ================
# fwgs_add_compile_options
#
# add_compile_options, but better with language specification
# ================
# */
macro(fwgs_add_compile_options lang)
	set(FLAGS ${ARGV})
	list(REMOVE_AT FLAGS 0) # Get rid of language

	string(REPLACE ";" " " FLAGS_STR "${FLAGS}")

	if(${lang} STREQUAL "C")
		fwgs_debug(STATUS "Adding ${FLAGS_STR} for both C/CXX")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS_STR}")
		set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${FLAGS_STR}")
	elseif(${lang} STREQUAL "CONLY")
		fwgs_debug(STATUS "Adding ${FLAGS_STR} for C ONLY")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS_STR}")
	elseif(${lang} STREQUAL "CXX")
		fwgs_debug(STATUS "Adding ${FLAGS_STR} for CXX")
		set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${FLAGS_STR}")
	endif()
endmacro()

# /*
# ================
# fwgs_fix_default_msvc_settings
#
# Tweaks CMake's default compiler/linker settings to suit Google Test's needs.
#
# This must be a macro(), as inside a function string() can only
# update variables in the function scope.
# ================
# */
macro(fwgs_fix_default_msvc_settings)
	if (MSVC)
		# For MSVC, CMake sets certain flags to defaults we want to override.
		# This replacement code is taken from sample in the CMake Wiki at
		# http://www.cmake.org/Wiki/CMake_FAQ#Dynamic_Replace.
		foreach (flag_var
			CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
			CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
			CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
	        # When Google Test is built as a shared library, it should also use
	        # shared runtime libraries.  Otherwise, it may end up with multiple
	        # copies of runtime library data in different modules, resulting in
	        # hard-to-find crashes. When it is built as a static library, it is
	        # preferable to use CRT as static libraries, as we don't have to rely
	        # on CRT DLLs being available. CMake always defaults to using shared
	        # CRT libraries, so we override that default here.
			string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
		endforeach()
	endif()
endmacro()

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
                set(SDL2_DEVELPKG "VC.zip")
                set(SDL2_SUBDIR "SDL2-${SDL2_VER}")
                set(SDL2_ARCHIVE "SDL2.zip")
                if(MINGW)
                        if(XASH_64BIT)
                                set(SDL2_SUBDIR "SDL2-${SDL2_VER}/x86_64-w64-mingw32")
                        else()
                                set(SDL2_SUBDIR "SDL2-${SDL2_VER}/i686-w64-mingw32")
                        endif()
                        set(SDL2_DEVELPKG "mingw.tar.gz")
                        set(SDL2_ARCHIVE "SDL2.tar.gz")
                endif()

                set(SDL2_DOWNLOAD_URL "http://libsdl.org/release/SDL2-devel-${SDL2_VER}-${SDL2_DEVELPKG}")

                fwgs_library_dependency(${tgt} SDL2 ${SDL2_DOWNLOAD_URL} ${SDL2_ARCHIVE} SDL2_PATH ${SDL2_SUBDIR})
        else()
                # SDL2 doesn't provide dev packages for nonWin32 targets
                fwgs_library_dependency(${tgt} SDL2)
        endif()
endmacro()

# Code below is taken from https://github.com/axr/solar-cmake/blob/master/TargetArch.cmake
# Modified by a1batross

set(archdetect_c_code "
#if defined(__arm__) || defined(__TARGET_ARCH_ARM)
    #if defined(__ARM_ARCH_7__) \\
	    || defined(__ARM_ARCH_7A__) \\
		|| defined(__ARM_ARCH_7R__) \\
		|| defined(__ARM_ARCH_7M__) \\
		|| (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM-0 >= 7)
		#error cmake_ARCH armv7
		#elif defined(__ARM_ARCH_6__) \\
		|| defined(__ARM_ARCH_6J__) \\
		|| defined(__ARM_ARCH_6T2__) \\
		|| defined(__ARM_ARCH_6Z__) \\
		|| defined(__ARM_ARCH_6K__) \\
		|| defined(__ARM_ARCH_6ZK__) \\
		|| defined(__ARM_ARCH_6M__) \\
		|| (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM-0 >= 6)
		#error cmake_ARCH armv6
		#elif defined(__ARM_ARCH_5TEJ__) \\
		|| (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM-0 >= 5)
		#error cmake_ARCH armv5
		#else
		#error cmake_ARCH arm
		#endif
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
    #error cmake_ARCH i386
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
    #error cmake_ARCH x86_64
#elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)
    #error cmake_ARCH ia64
#elif defined(__ppc__) || defined(__ppc) || defined(__powerpc__) \\
      || defined(_ARCH_COM) || defined(_ARCH_PWR) || defined(_ARCH_PPC)  \\
	  || defined(_M_MPPC) || defined(_M_PPC)
	#if defined(__ppc64__) || defined(__powerpc64__) || defined(__64BIT__)
	    #error cmake_ARCH ppc64
		#else
		#error cmake_ARCH ppc
		#endif
#endif

#error cmake_ARCH unknown
")

# Set ppc_support to TRUE before including this file or ppc and ppc64
# will be treated as invalid architectures since they are no longer supported by Apple

function(target_architecture output_var)
	if(APPLE AND CMAKE_OSX_ARCHITECTURES)
		# On OS X we use CMAKE_OSX_ARCHITECTURES *if* it was set
		# First let's normalize the order of the values

		# Note that it's not possible to compile PowerPC applications if you are using
		# the OS X SDK version 10.6 or later - you'll need 10.4/10.5 for that, so we
		# disable it by default
		# See this page for more information:
		# http://stackoverflow.com/questions/5333490/how-can-we-restore-ppc-ppc64-as-well-as-full-10-4-10-5-sdk-support-to-xcode-4

		# Architecture defaults to i386 or ppc on OS X 10.5 and earlier, depending on the CPU type detected at runtime.
		# On OS X 10.6+ the default is x86_64 if the CPU supports it, i386 otherwise.

		foreach(osx_arch ${CMAKE_OSX_ARCHITECTURES})
			if("${osx_arch}" STREQUAL "ppc" AND ppc_support)
				set(osx_arch_ppc TRUE)
			elseif("${osx_arch}" STREQUAL "i386")
				set(osx_arch_i386 TRUE)
			elseif("${osx_arch}" STREQUAL "x86_64")
				set(osx_arch_x86_64 TRUE)
			elseif("${osx_arch}" STREQUAL "ppc64" AND ppc_support)
				set(osx_arch_ppc64 TRUE)
			else()
				message(FATAL_ERROR "Invalid OS X arch name: ${osx_arch}")
			endif()
		endforeach()

		# Now add all the architectures in our normalized order
		if(osx_arch_ppc)
			list(APPEND ARCH ppc)
		endif()

		if(osx_arch_i386)
			list(APPEND ARCH i386)
		endif()

		if(osx_arch_x86_64)
			list(APPEND ARCH x86_64)
		endif()

		if(osx_arch_ppc64)
			list(APPEND ARCH ppc64)
		endif()
	else()
		file(WRITE "${CMAKE_BINARY_DIR}/arch.c" "${archdetect_c_code}")

		enable_language(C)

		# Detect the architecture in a rather creative way...
		# This compiles a small C program which is a series of ifdefs that selects a
		# particular #error preprocessor directive whose message string contains the
		# target architecture. The program will always fail to compile (both because
		# file is not a valid C program, and obviously because of the presence of the
		# #error preprocessor directives... but by exploiting the preprocessor in this
		# way, we can detect the correct target architecture even when cross-compiling,
		# since the program itself never needs to be run (only the compiler/preprocessor)
		try_run(
			run_result_unused
			compile_result_unused
			"${CMAKE_BINARY_DIR}"
			"${CMAKE_BINARY_DIR}/arch.c"
			COMPILE_OUTPUT_VARIABLE ARCH
			CMAKE_FLAGS CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
			)

		# Parse the architecture name from the compiler output
		string(REGEX MATCH "cmake_ARCH ([a-zA-Z0-9_]+)" ARCH "${ARCH}")

		# Get rid of the value marker leaving just the architecture name
		string(REPLACE "cmake_ARCH " "" ARCH "${ARCH}")

		# If we are compiling with an unknown architecture this variable should
		# already be set to "unknown" but in the case that it's empty (i.e. due
		# to a typo in the code), then set it to unknown
		if (NOT ARCH)
			set(ARCH unknown)
		endif()
	endif()

	message(STATUS "Target architecture: ${ARCH}")
	set(${output_var} "${ARCH}" PARENT_SCOPE)
endfunction()

# Set ARCH global variable
target_architecture(ARCH)

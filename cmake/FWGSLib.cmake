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
			PERMISSIONS OWNER_READ OWNER_READ OWNER_EXECUTE
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
# fwgs_set_default_properties
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
	set_target_properties(${tgt} PROPERTIES
		POSITION_INDEPENDENT_CODE 1)
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

		find_package(${pkgname}) # First try to find it in system!
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

### from https://cmake.org/Bug/view.php?id=15804
# But then modified to fix some problems such as 
#	- using <PREFIX>_LIBDIR to find libraries correctly
#	- using pkg_search_module() instead of pkg_check_modules()
#	- not defining the import target when the library was not found
# Hopefully, cmake will come up with a way to correctly apply what pkg_search_module() found to a target, libraries and all and this can go away.
################################################
#.rst:
# PkgImportModule
# ---------------
#
# CMake commands to create imported targets from pkg-config modules.

#=============================================================================
# Copyright 2016 Lautsprecher Teufel GmbH
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_package(PkgConfig REQUIRED)

# ::
#
#     pkg_import_module(<name> args)
#
# Creates an imported target named <name> from a pkg-config module. All
# remaining arguments are passed through to pkg_search_module(). See the
# documentation of the FindPkgConfig module for information.
#
# Example usage:
#
#   pkg_import_module(Archive::Archive REQUIRED libarchive>=3.0)
#   pkg_import_module(Avahi::GObject REQUIRED avahi-gobject)
#
function(pkg_import_module name)
    set(prefix _${name})
    pkg_search_module("${prefix}" ${ARGN})

	if (NOT ${prefix}_FOUND)
		return()
	endif()

	set(dirs ${${prefix}_LIBRARY_DIRS})
	if (NOT "${${prefix}_LIBDIR}" STREQUAL "")
		list(APPEND dirs ${${prefix}_LIBDIR})
	endif()

	#message("dirs: ${dirs}")

    _pkg_import_module_add_imported_target(
        ${name}
        "${${prefix}_CFLAGS_OTHER}"
        "${${prefix}_INCLUDE_DIRS}"
        "${dirs}"
        "${${prefix}_LIBRARIES}"
        )
endfunction()

function(_pkg_import_module_find_libraries output_var prefix library_names library_dirs)
    foreach(libname ${library_names})
        # find_library stores its result in the cache, so we must pass a unique
        # variable name for each library that we look for.
        set(library_path_var "${prefix}_LIBRARY_${libname}")
		#message("************ looking for ${library_path_var} as ${libname} in ${library_dirs} /usr/lib/x86_64-linux-gnu")
        find_library(${library_path_var} ${libname} HINTS ${library_dirs} /usr/lib/x86_64-linux-gnu)
		#message("************ found: ${${library_path_var}}")
		if (EXISTS "${${library_path_var}}")
    	    list(APPEND library_paths "${${library_path_var}}")
		else()
			message(FATAL_ERROR "failed to locate a library named '${libname}' in paths '${library_dirs}'")
		endif()
    endforeach()
    set(${output_var} ${library_paths} PARENT_SCOPE)
endfunction()

function(_pkg_import_module_add_imported_target name compile_options include_dirs library_dirs libraries)
    # FIXME: we should really detect whether it's SHARED or STATIC, instead of
    # assuming SHARED. We can't just use UNKNOWN because nothing can link
    # against it then.
    add_library(${name} SHARED IMPORTED)

    # We must pass in the absolute paths to the libraries, otherwise, when
    # the library is installed in a non-standard prefix the linker won't be
    # able to find the libraries. CMake doesn't provide a way for us to keep
    # track of the library search path for a specific target, so we have to
    # do it this way.
    _pkg_import_module_find_libraries(libraries_full_paths
        ${name} "${libraries}" "${library_dirs}")

    list(GET libraries_full_paths 0 imported_location)

    set_target_properties(${name} PROPERTIES
        IMPORTED_LOCATION ${imported_location}
        INTERFACE_COMPILE_OPTIONS "${compile_options}"
        INTERFACE_INCLUDE_DIRECTORIES "${include_dirs}"
        INTERFACE_LINK_LIBRARIES "${libraries_full_paths}"
        )
endfunction()

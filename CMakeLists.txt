cmake_minimum_required(VERSION 3.1)	
project(rezound)


# consider that we don't even need this file if the top-level sets the parameters
set(REZOUND_PACKAGE "rezound")
SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "13")
SET(VERSION_PATCH "1")
set(REZOUND_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(PACKAGE_BUGREPORT "https://sourceforge.net/p/rezound/bugs/")
set(PACKAGE_URL "https://sourceforge.net/p/rezound/bugs/")
#set(INSTALL_PREFIX "/usr/local" CACHE STRING "The the installation prefix (/usr/local by default)")
set(DATA_DIR "${CMAKE_INSTALL_PREFIX}/share" CACHE STRING "The directory where ancillary data files are found")
set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")


# define SRC_ROOT as the absolute path to the top of the tree
get_filename_component(SRC_ROOT "${CMAKE_CURRENT_LIST_DIR}" REALPATH)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")

include (CheckIncludeFile)
include(PkgImportModule)
find_package(PkgConfig)


set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(CMAKE_COLOR_MAKEFILE TRUE)

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "Setting CMAKE_BUILD_TYPE to Debug by default")
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_LDFLAGS_FLAGS} -Xlinker --allow-multiple-definition")

# (enabled by default on x64)
## enable large file support
#add_definitions(-D_LARGEFILE_SOURCE)
#execute_process(COMMAND getconf LFS_CFLAGS OUTPUT_VARIABLE LFS_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
#if (NOT "${LFS_CFLAGS}" STREQUAL "")
#	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${LFS_CFLAGS})
#endif()

	# TODO see about eliminating all but this first one and let everything include src/...
include_directories("${SRC_ROOT}")
include_directories("${SRC_ROOT}/src/misc")
include_directories("${SRC_ROOT}/src/PoolFile")
include_directories("${SRC_ROOT}/src/backend")

# where we'll put generated files (e.g. config/config.h)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")

# warn if this is building on a 32bit OS.. not well supported
if (NOT "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
	message(WARNING "Architectures other than x86_64 are NOT recommended.  Particularly 32bit architectures may not be able to open large files")
endif()


execute_process(COMMAND which cdrdao OUTPUT_VARIABLE x OUTPUT_STRIP_TRAILING_WHITESPACE)
if ("${x}" STREQUAL "")
	message(WARNING "cdrdao not found on \$PATH -- this is necessary for burning audio files to disc -- http://cdrdao.sourceforge.net (install: cdrdao)")
endif()

include (TestBigEndian)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

# internal data type for audio samples
set(SAMPLE_TYPE "float" CACHE STRING "The internal data-type used to represent an audio sample")
set(SAMPLE_TYPE_S16 OFF)
set(SAMPLE_TYPE_FLOAT OFF)
set_property(CACHE SAMPLE_TYPE PROPERTY STRINGS int16 float)

if ("${SAMPLE_TYPE}" STREQUAL "int16")
	message(STATUS "setting the internal sample type to signed 16 bit")
	set(SAMPLE_TYPE_S16 ON)
elseif ("${SAMPLE_TYPE}" STREQUAL "float")
	message(STATUS "setting the internal sample type to signed 32 bit floating point")
	set(SAMPLE_TYPE_FLOAT ON)
else()
	message(FATAL_ERROR "sample-type must be defined as int16 or float -- \"${sampleType}\" is invalid")
endif()


### NLS #############################################################

set(ENABLE_NLS OFF CACHE INTERNAL "")
set(HAVE_LIBINTL OFF CACHE INTERNAL "")
include(FindIntl)
if (Intl_FOUND)
	set(ENABLE_NLS ON)
	set(HAVE_LIBINTL ON)
	message(STATUS "libintl found.  Internaltionalization will be enabled")

else()
	set(ENABLE_NLS OFF)
	set(HAVE_LIBINTL OFF)
	message(WARNING "libintl not found.  Internaltionalization will not be enabled (install: gettext)")
endif()






add_subdirectory(po)
add_subdirectory(src/misc)
add_subdirectory(src/PoolFile)
add_subdirectory(src/backend)
add_subdirectory(src/frontend_fox)

configure_file(${CMAKE_CURRENT_LIST_DIR}/config/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/config/config.h)


# ### local install #########################################################

# installation (other than the binary in the frontend src folder)
install(
	FILES 
		share/presets.dat
		share/menu.dat
		share/key_bindings.dat
		share/impulse_hall1.wav
		src/images/icon_logo_32.gif
	DESTINATION share/${REZOUND_PACKAGE}
)

install(
	FILES 
		docs/AUTHORS
		docs/COPYING
		docs/NEWS
		docs/README
		docs/FrontendFoxFeatures.txt
		docs/Features.txt
	DESTINATION doc/${REZOUND_PACKAGE}
)

# TODO create kde desktop files like Makefile.am used to do .. use files in packaging/generic_rpm/ ... do gnome too

# uninstall target
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
)


# ### packaging ##############################################

set(CPACK_GENERATOR "STGZ;DEB")

set(CPACK_PACKAGE_NAME "${REZOUND_PACKAGE}")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ReZound audio file editor")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_PACKAGE_VENDOR "Me, myself, and I") # determine
set(CPACK_PACKAGE_CONTACT "me@example.com") # determine
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/docs/COPYING")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
set(CPACK_STRIP_FILES "bin/MyExecutable")
set(CPACK_SOURCE_STRIP_FILES "")
set(CPACK_PACKAGE_EXECUTABLES "MyExecutable" "My Executable")

set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64") # obtained from `dpkg --print-architecture` .. 32bit builds are not recommended any longer
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.0), libgcc1 (>= 1:3.4.2-12)")  TODO figure out a way to use dpkg-shlibdeps  or  continue working on my shell script compute_deb_deps.sh
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "lame;cdrdao")

# TODO this cpack stuff is totally cool.. but I will need to work out how the rezound binary can locate the data-dir since these packages all seem to assume relocatability.. perhaps there's a configuration that it writes as to where it installed stuff
include(CPack)


# source tarball (tar up all the files tracked by git in the current checkout)
add_custom_target(tarball
	cd "${SRC_ROOT}" \;
	git ls-files . -z | tar -cvj --exclude WORK --transform 's|^|${REZOUND_PACKAGE}-${REZOUND_VERSION}/|' --null --files-from=- -f "${CMAKE_CURRENT_BINARY_DIR}/${REZOUND_PACKAGE}-${REZOUND_VERSION}.tar.bz2"
)



cmake_minimum_required (VERSION 2.8.8)
cmake_policy(SET CMP0043 NEW)
Project(ktools CXX)

set(PACKAGE_VERSION_MAJOR 4)
set(PACKAGE_VERSION_MINOR 4)
set(PACKAGE_VERSION_PATCH 0)
set(PACKAGE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")

OPTION(DISABLE_CPU_EXTENSIONS "Disable CPU extensions (portable build)." OFF)
OPTION(BUNDLED_DEPENDENCIES "Build under the assumption dependencies will be bundled with the executable." OFF)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/out_of_source_build.cmake)
ENSURE_OUT_OF_SOURCE_BUILD( "${PROJECT_SOURCE_DIR}/build" )

ENABLE_LANGUAGE(C)


set( COMMON_INCLUDE_DIRS "" )
set( LINK_DIRS "" )
set( COMMON_LIBS "ktool_common" "squish" "pugixml" )

set( ktech_LIBS "" )
set( krane_LIBS "" )


list( APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" )
list( APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/thirdparty" )

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckStructHasMember)

include(LibFindMacros)
include(integer_types)
include(warnings)
include(compiler_specific)
include(OptimizeForArchitecture)


set(CMAKE_BUILD_TYPE Release)


if(NOT DISABLE_CPU_EXTENSIONS)
	OptimizeForArchitecture()
endif()
if(BUNDLED_DEPENDENCIES)
	add_definitions(-DBUNDLED_DEPENDENCIES=1)
endif()


add_subdirectory(lib)


FIND_PACKAGE(ImageMagick COMPONENTS Magick++ MagickWand MagickCore REQUIRED)
LIBFIND_PKG_CHECK_MODULES(Magick++_PKGCONF Magick++)
if("${Magick++_PKGCONF_CFLAGS}" STREQUAL "")
	if("${ImageMagick_LIBRARIES}" MATCHES "HDRI")
		add_definitions( "-DMAGICKCORE_HDRI_ENABLE=1" )
	else()
		add_definitions( "-DMAGICKCORE_HDRI_ENABLE=0" )
	endif()
	if("${ImageMagick_LIBRARIES}" MATCHES "Q16")
		add_definitions( "-DMAGICKCORE_QUANTUM_DEPTH=16" )
	else()
		add_definitions( "-DMAGICKCORE_QUANTUM_DEPTH=8" )
	endif()
else()
	foreach(f ${Magick++_PKGCONF_CFLAGS})
		set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${f}" )
	endforeach()
endif()
foreach(f ${Magick++_PKGCONF_LDFLAGS})
	set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${f}" )
endforeach()
list( APPEND COMMON_INCLUDE_DIRS "${ImageMagick_INCLUDE_DIRS}" )
list( APPEND COMMON_LIBS "${ImageMagick_LIBRARIES}" )


FIND_PACKAGE(LibZip)
if(LIBZIP_FOUND)
	list( APPEND COMMON_INCLUDE_DIRS "${LIBZIP_INCLUDE_DIR}" )
	list( APPEND COMMON_LIBS "${LIBZIP_LIBRARY}" )

	set(HAVE_LIBZIP 1)

	FIND_PACKAGE(ZLIB)
	if(ZLIB_FOUND)
		#list( APPEND COMMON_INCLUDE_DIRS "${ZLIB_INCLUDE_DIR}" )
		list( APPEND COMMON_LIBS "${ZLIB_LIBRARY}" )
	endif(ZLIB_FOUND)
endif(LIBZIP_FOUND)


CHECK_LIBRARY_EXISTS(m floor "" HAVE_MATH_LIB) 
if(HAVE_MATH_LIB)
	list( APPEND COMMON_LIBS "m" )
endif(HAVE_MATH_LIB)

list( APPEND ktech_LIBS "${COMMON_LIBS}" )
list( APPEND krane_LIBS "${COMMON_LIBS}" )


CHECK_INCLUDE_FILE_CXX (cstddef HAVE_CSTDDEF)
if(NOT HAVE_CSTDDEF)
	CHECK_INCLUDE_FILE (stddef.h HAVE_STDDEF_H)
	if(NOT HAVE_STDDEF_H)
		message(FATAL_ERROR "Failed to find cstddef or stddef.h header.")
	endif()
endif()
#
CHECK_INCLUDE_FILE (stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILE (inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE (sys/types.h HAVE_SYS_TYPES_H)
if(NOT HAVE_STDINT_H AND NOT HAVE_INTTYPES_H AND NOT HAVE_SYS_TYPES_H)
	message(FATAL_ERROR "Failed to find a standard header defining integral types.")
endif()
#
CHECK_INCLUDE_FILE_CXX (sstream HAVE_SSTREAM)
if(NOT HAVE_SSTREAM)
	CHECK_INCLUDE_FILE_CXX (strstream HAVE_STRSTREAM)
	if(NOT HAVE_STRSTREAM)
		CHECK_INCLUDE_FILE_CXX (strstream.h HAVE_STRSTREAM_H)
		if(NOT HAVE_STRSTREAM_H)
			message(FATAL_ERROR "Failed to find a sstream or strstream header.")
		endif()
	endif()
endif()
#
if(HAVE_STRSTREAM OR HAVE_STRSTREAM_H)
	set(HAVE_CLASS_STRSTREAM TRUE)
endif()
if(HAVE_SSTREAM)
	set(HAVE_CLASS_STRINGSTREAM TRUE)
endif()
#
#if(HAVE_STRSTREAM)
#	set(CMAKE_EXTRA_INCLUDE_FILES "strstream")
#elseif(HAVE_STRSTREAM_H)
#	set(CMAKE_EXTRA_INCLUDE_FILES "strstream.h")
#endif()
#CHECK_TYPE_SIZE("std::ostrstream" CLASS_STRSTREAM LANGUAGE CXX)
#set(CMAKE_EXTRA_INCLUDE_FILES)
#
#set(CMAKE_EXTRA_INCLUDE_FILES "sstream")
#CHECK_TYPE_SIZE("std::stringstream" SSTREAM LANGUAGE CXX)
#set(CMAKE_EXTRA_INCLUDE_FILES)
#
CHECK_TYPE_SIZE("long long" LONG_LONG)
#
CHECK_SYMBOL_EXISTS("snprintf" "stdio.h" HAVE_SNPRINTF)
if(NOT HAVE_SNPRINTF)
	CHECK_SYMBOL_EXISTS("_snprintf" "stdio.h" HAVE__SNPRINTF)
endif()
#
set(CMAKE_EXTRA_INCLUDE_FILES "sys/stat.h")
CHECK_TYPE_SIZE("mode_t" MODE_T)
set(CMAKE_EXTRA_INCLUDE_FILES)
#
CHECK_CXX_SOURCE_COMPILES (
  "int test (void *restrict x); int main (void) {return 0;}"
	HAVE_RESTRICT) 
if(NOT HAVE_RESTRICT)
	CHECK_CXX_SOURCE_COMPILES (
	  "int test (void *__restrict x); int main (void) {return 0;}"
		HAVE___RESTRICT) 
	if(NOT HAVE___RESTRICT)
		CHECK_CXX_SOURCE_COMPILES (
		  "int test (void *__restrict__ x); int main (void) {return 0;}"
			HAVE___RESTRICT__) 
	endif()
endif()
#
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
list( APPEND COMMON_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} )


add_subdirectory(src)


list( APPEND COMMON_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/common ${PROJECT_SOURCE_DIR}/lib )
set( KTECH_INCLUDE_DIRS ${COMMON_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/ktech )
set( KRANE_INCLUDE_DIRS ${COMMON_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/krane )
set( LIBKTOOL_INCLUDE_DIRS ${COMMON_INCLUDE_DIRS} )

link_directories( ${LINK_DIRS} )

# add_executable(<name> [WIN32] [MACOSX_BUNDLE] ...
if(NOT WIN32)
	add_executable(ktech ${ktech_SOURCES} ${ktech_HEADERS})
	add_executable(krane ${krane_SOURCES} ${krane_HEADERS})
else()
	add_executable(ktech WIN32 ${ktech_SOURCES} ${ktech_HEADERS})
	add_executable(krane WIN32 ${krane_SOURCES} ${krane_HEADERS})
endif()
add_library(ktool_common STATIC ${ktool_common_SOURCES} ${ktool_common_HEADERS})

target_link_libraries( ktech ${ktech_LIBS} )
target_link_libraries( krane ${krane_LIBS} )

set_property( TARGET ktech PROPERTY INCLUDE_DIRECTORIES "${KTECH_INCLUDE_DIRS}" )
set_property( TARGET krane PROPERTY INCLUDE_DIRECTORIES "${KRANE_INCLUDE_DIRS}" )
set_property( TARGET ktool_common PROPERTY INCLUDE_DIRECTORIES "${LIBKTOOL_INCLUDE_DIRS}" )

if(MSVC)
   set_target_properties(ktech PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
   set_target_properties(ktech PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
   set_target_properties(ktech PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
   set_target_properties(ktech PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
   set_target_properties(ktech PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
   set_target_properties(ktech PROPERTIES COMPILE_DEFINITIONS_RELEASE "_CONSOLE")
   set_target_properties(ktech PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
   set_target_properties(ktech PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "_CONSOLE")

   set_target_properties(krane PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
   set_target_properties(krane PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
   set_target_properties(krane PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
   set_target_properties(krane PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
   set_target_properties(krane PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
   set_target_properties(krane PROPERTIES COMPILE_DEFINITIONS_RELEASE "_CONSOLE")
   set_target_properties(krane PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
   set_target_properties(krane PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "_CONSOLE")
endif(MSVC)


INSTALL(TARGETS ktech RUNTIME DESTINATION bin)
INSTALL(TARGETS krane RUNTIME DESTINATION bin)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

#
# Packing stuff
#
set(CPACK_PACKAGE_VERSION_MAJOR "${PACKAGE_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PACKAGE_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross platform Don't Starve modding tools.")
set(CPACK_PACKAGE_VENDOR "simplex")
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PACKAGE_VERSION}")
set(CPACK_SOURCE_GENERATOR "TGZ")
# The following components are regex's to match anywhere (unless anchored)
# in absolute path + filename to find files or directories to be excluded
# from source tarball.
set(CPACK_SOURCE_IGNORE_FILES
"~$"
"/[.].+$"
"^${CMAKE_CURRENT_SOURCE_DIR}/local/"
"^${CMAKE_CURRENT_SOURCE_DIR}/deps/"
"^${CMAKE_CURRENT_SOURCE_DIR}/build/"
"^${CMAKE_CURRENT_SOURCE_DIR}/config.h$"
"/cmake_install.cmake$"
"/CMakeFiles/"
"/Makefile$"
"^${CMAKE_BINARY_DIR}/.*"
)
include(CPack)

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
#add_custom_target(bindist COMMAND ${CMAKE_MAKE_PROGRAM} package)
add_custom_target(distclean
	COMMAND rm -rf CMakeCache.txt CPackConfig.cmake CPackSourceConfig.cmake config.h
	COMMAND -find . '\(' -name CMakeFiles -o -name cmake_install.cmake -o -name Makefile '\)' -exec rm -rf {} '\;'
	COMMAND rm -rf build/*
)

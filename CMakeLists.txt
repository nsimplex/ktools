cmake_minimum_required (VERSION 2.8.8)
Project(ktools CXX)

set(PACKAGE_VERSION_MAJOR 4)
set(PACKAGE_VERSION_MINOR 0)
set(PACKAGE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")

OPTION(DISABLE_CPU_EXTENSIONS "Disable CPU extensions (portable build)." OFF)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/out_of_source_build.cmake)
ENSURE_OUT_OF_SOURCE_BUILD( "${PROJECT_SOURCE_DIR}/build" )

ENABLE_LANGUAGE(C)


set( COMMON_INCLUDE_DIRS "" )
set( LINK_DIRS "" )
set( COMMON_LIBS "ktool_common" "squish" )

set( ktech_LIBS "" )
set( krane_LIBS "pugixml" )


include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(CheckFunctionExists)
include(CheckTypeSize)
include(CheckStructHasMember)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/thirdparty/LibFindMacros.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/integer_types.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/warnings.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compiler_specific.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/thirdparty/OptimizeForArchitecture.cmake)


set(CMAKE_BUILD_TYPE Release)
if(NOT DISABLE_CPU_EXTENSIONS)
	OptimizeForArchitecture()
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


CHECK_LIBRARY_EXISTS(m floor "" HAVE_MATH_LIB) 
if(HAVE_MATH_LIB)
	list( APPEND COMMON_LIBS "m" )
endif(HAVE_MATH_LIB)

list( APPEND ktech_LIBS "${COMMON_LIBS}" )
list( APPEND krane_LIBS "${COMMON_LIBS}" )


CHECK_INCLUDE_FILE_CXX (cstddef HAVE_CSTDDEF)
#
CHECK_INCLUDE_FILE (stddef.h HAVE_STDDEF_H)
CHECK_INCLUDE_FILE (stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILE (inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE (sys/types.h HAVE_SYS_TYPES_H)
#
CHECK_INCLUDE_FILE_CXX (strstream HAVE_STRSTREAM)
CHECK_INCLUDE_FILE_CXX (strstream.h HAVE_STRSTREAM_H)
#
CHECK_INCLUDE_FILE_CXX (sstream HAVE_SSTREAM)
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
set(CMAKE_EXTRA_INCLUDE_FILES "stdio.h")
CHECK_FUNCTION_EXISTS("snprintf" HAVE_SNPRINTF)
if(NOT HAVE_SNPRINTF)
	CHECK_FUNCTION_EXISTS("_snprintf", HAVE__SNPRINTF)
endif()
set(CMAKE_EXTRA_INCLUDE_FILES)
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


#
# Packing stuff
#
set(CPACK_PACKAGE_VERSION_MAJOR "${PACKAGE_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PACKAGE_VERSION_MINOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Don't Starve modding tools for *nix.")
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
"/local/"
"/deps/"
"/build/"
"/cmake_install.cmake$"
".*/cmake_install.cmake$"
"/CMakeFiles/"
"/Makefile$"
"${CMAKE_BINARY_DIR}/.*"
)
include(CPack)

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
add_custom_target(bindist COMMAND ${CMAKE_MAKE_PROGRAM} package)
add_custom_target(distclean
	COMMAND rm -rf CMakeCache.txt CPackConfig.cmake CPackSourceConfig.cmake
	COMMAND -find . '\(' -name CMakeFiles -o -name cmake_install.cmake -o -name Makefile '\)' -exec rm -rf {} '\;'
	COMMAND rm -rf build/*
)

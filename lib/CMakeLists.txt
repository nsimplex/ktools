if(SSE2_FOUND)
	set(BUILD_SQUISH_WITH_SSE2 ON)
elseif(NOT DISABLE_CPU_EXTENSIONS)
	set(BUILD_SQUISH_WITH_SSE2 OFF)
endif()

add_subdirectory( squish )
add_subdirectory( pugixml )

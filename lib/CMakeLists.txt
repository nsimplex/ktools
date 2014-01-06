if(SSE2_FOUND)
	set(BUILD_SQUISH_WITH_SSE2 ON CACHE BOOL "")
else()
	set(BUILD_SQUISH_WITH_SSE2 OFF CACHE BOOL "")
endif()

add_subdirectory( squish )

set( ktech_LIBS "${ktech_LIBS};squish" PARENT_SCOPE )

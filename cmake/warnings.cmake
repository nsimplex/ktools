include(AddCompilerFlag)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic")
	AddCompilerFlag("-Wno-unused-local-typedefs" CXX_FLAGS CMAKE_CXX_FLAGS)
	AddCompilerFlag("-Wno-long-long" CXX_FLAGS CMAKE_CXX_FLAGS)
elseif(MSVC)
	# Force to always compile with W4
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
      string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
endif()

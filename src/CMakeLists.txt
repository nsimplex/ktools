set( local_ktech_SOURCES 
	ktech.cpp ktech_options.cpp
)

set( local_ktech_HEADERS
	ktech.hpp ktech_common.hpp ktech_bit_op.hpp image_operations.hpp ktech_options.hpp ktech_options_output.hpp
	ktex/ktex.cpp ktex/specs.cpp
	ktex/ktex.hpp ktex/specs.hpp ktex/headerfield_specs.hpp ktex/io.hpp
	compat.hpp compat/common.hpp compat/posix.hpp compat/fs.hpp
)


set( prefixed_ktech_SOURCES )
set( prefixed_ktech_HEADERS )

foreach(src ${local_ktech_SOURCES})
	list(APPEND prefixed_ktech_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${src})
endforeach()
foreach(hdr ${local_ktech_HEADERS})
	list(APPEND prefixed_ktech_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/${hdr})
endforeach()

set( ktech_SOURCES "${prefixed_ktech_SOURCES}" PARENT_SCOPE )
set( ktech_HEADERS "${prefixed_ktech_HEADERS}" PARENT_SCOPE )

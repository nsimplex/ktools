set( local_ktech_SOURCES 
	ktech/ktech.cpp ktech/ktech_options.cpp
	common/ktex/ktex.cpp common/ktex/specs.cpp
)

set( local_ktech_HEADERS
	ktech/ktech.hpp ktech/ktech_common.hpp 
	ktech/ktech_options.hpp ktech/ktech_options_output.hpp
	common/compat.hpp common/compat/common.hpp common/compat/posix.hpp common/compat/fs.hpp
	common/ktools_bit_op.hpp common/image_operations.hpp 
	common/ktex/ktex.hpp common/ktex/specs.hpp common/ktex/headerfield_specs.hpp common/ktex/io.hpp
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

set( local_ktech_SOURCES 
	ktech/ktech.cpp ktech/ktech_options.cpp
)

set( local_ktech_HEADERS
	ktech/ktech.hpp ktech/ktech_common.hpp 
	ktech/image_processing.hpp
	ktech/ktech_options.hpp
	common/compat.hpp common/compat/common.hpp common/compat/posix.hpp common/compat/fs.hpp
	common/metaprogramming.hpp common/ktools_common.hpp
	common/ktools_bit_op.hpp common/image_operations.hpp common/binary_io_utils.hpp
	common/ktex/ktex.hpp common/ktex/specs.hpp common/ktex/headerfield_specs.hpp
	common/file_abstraction.hpp
	common/ktools_options_customization.hpp
)


set( local_krane_SOURCES
	krane/krane.cpp krane/krane_options.cpp
	krane/kbuild.cpp krane/kanim.cpp
	krane/kbuild_serialize.cpp krane/kanim_serialize.cpp
	krane/scml.cpp
)

set( local_krane_HEADERS
	krane/krane.hpp krane/krane_common.hpp krane/krane_options.hpp
	krane/kbuild.hpp krane/kanim.hpp
	krane/scml.hpp
	krane/krane_options.hpp
	common/compat.hpp common/compat/common.hpp common/compat/posix.hpp common/compat/fs.hpp
	common/metaprogramming.hpp common/ktools_common.hpp
	common/ktools_bit_op.hpp common/image_operations.hpp common/binary_io_utils.hpp
	common/ktex/ktex.hpp common/ktex/specs.hpp common/ktex/headerfield_specs.hpp
	common/file_abstraction.hpp
	common/ktools_options_customization.hpp
)


set( local_ktool_common_SOURCES
	common/ktools_common.cpp
	common/file_abstraction.cpp
	common/ktex/ktex.cpp common/ktex/specs.cpp
	common/atlas.cpp
	common/ktools_options_customization.cpp
)

set( local_ktool_common_HEADERS
	common/metaprogramming.hpp common/ktools_common.hpp
	common/ktools_bit_op.hpp common/image_operations.hpp common/binary_io_utils.hpp
	common/file_abstraction.hpp
	common/ktex/ktex.hpp common/ktex/specs.hpp common/ktex/headerfield_specs.hpp
	common/atlas.hpp
	common/ktools_options_customization.hpp
)


set( prefixed_ktech_SOURCES )
set( prefixed_ktech_HEADERS )

foreach(src ${local_ktech_SOURCES})
	list(APPEND prefixed_ktech_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${src})
endforeach()
foreach(hdr ${local_ktech_HEADERS})
	list(APPEND prefixed_ktech_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/${hdr})
endforeach()


set( prefixed_krane_SOURCES )
set( prefixed_krane_HEADERS )

foreach(src ${local_krane_SOURCES})
	list(APPEND prefixed_krane_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${src})
endforeach()
foreach(hdr ${local_krane_HEADERS})
	list(APPEND prefixed_krane_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/${hdr})
endforeach()


set( prefixed_ktool_common_SOURCES )
set( prefixed_ktool_common_HEADERS )

foreach(src ${local_ktool_common_SOURCES})
	list(APPEND prefixed_ktool_common_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${src})
endforeach()
foreach(hdr ${local_ktool_common_HEADERS})
	list(APPEND prefixed_ktool_common_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/${hdr})
endforeach()


set( ktech_SOURCES "${prefixed_ktech_SOURCES}" PARENT_SCOPE )
set( ktech_HEADERS "${prefixed_ktech_HEADERS}" PARENT_SCOPE )

set( krane_SOURCES "${prefixed_krane_SOURCES}" PARENT_SCOPE )
set( krane_HEADERS "${prefixed_krane_HEADERS}" PARENT_SCOPE )

set( ktool_common_SOURCES "${prefixed_ktool_common_SOURCES}" PARENT_SCOPE )
set( ktool_common_HEADERS "${prefixed_ktool_common_HEADERS}" PARENT_SCOPE )

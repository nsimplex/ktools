set(HEADERS pugixml.hpp pugiconfig.hpp)
set(SOURCES ${HEADERS} pugixml.cpp)

add_library(pugixml STATIC ${SOURCES})

set_target_properties(pugixml PROPERTIES VERSION 1.4 SOVERSION 1)

set_property( TARGET pugixml PROPERTY INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" )

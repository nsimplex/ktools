# Ensures that we do an out of source build

MACRO(ENSURE_OUT_OF_SOURCE_BUILD DEFAULT_DIR)
    STRING(COMPARE EQUAL "${CMAKE_SOURCE_DIR}"
"${CMAKE_BINARY_DIR}" insource)
    GET_FILENAME_COMPONENT(PARENTDIR ${CMAKE_SOURCE_DIR} PATH)
    STRING(COMPARE EQUAL "${CMAKE_SOURCE_DIR}"
"${PARENTDIR}" insourcesubdir)
    IF(insource OR insourcesubdir)
		set( CMAKE_BINARY_DIR "${DEFAULT_DIR}" )
    ENDIF(insource OR insourcesubdir)
ENDMACRO()

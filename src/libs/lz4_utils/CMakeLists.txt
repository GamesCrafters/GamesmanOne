set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/lz4_utils.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lz4_utils.c)

add_library(lz4_utils STATIC ${HEADERS} ${SOURCES})

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/xzra.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/xzra.c)

add_library(xzra STATIC ${HEADERS} ${SOURCES})
target_link_libraries(xzra PRIVATE common_flags)
target_link_libraries(xzra PRIVATE lzma_wrapper)

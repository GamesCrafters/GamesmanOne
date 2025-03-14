set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/lz4_utils.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lz4_utils.c)

add_library(lz4_utils STATIC ${HEADERS} ${SOURCES})
target_link_libraries(lz4_utils PRIVATE common_flags)
find_package(lz4 1.10.0 REQUIRED CONFIG)
target_link_libraries(lz4_utils PRIVATE lz4::lz4)

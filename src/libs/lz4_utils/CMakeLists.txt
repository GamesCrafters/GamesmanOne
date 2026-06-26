set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/lz4_utils.h)

set_source_files_properties(
    lz4_utils.c
    PROPERTIES COMPILE_FLAGS "-Wno-analyzer-use-of-uninitialized-value"
)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lz4_utils.c)

add_library(lz4_utils STATIC ${HEADERS} ${SOURCES})
target_link_libraries(lz4_utils PRIVATE common_flags)
target_link_libraries(lz4_utils PRIVATE lz4)

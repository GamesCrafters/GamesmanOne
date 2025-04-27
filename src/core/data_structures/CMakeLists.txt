set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/bitstream.h
    ${CMAKE_CURRENT_SOURCE_DIR}/concurrent_bitset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cstring.h
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_hash_map_sc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_hash_map.h
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_hash_set.h
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_queue.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/bitstream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/concurrent_bitset.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cstring.c
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_array.c
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_hash_map_sc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_hash_map.c
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_hash_set.c
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_queue.c)

add_library(data_structures STATIC ${HEADERS} ${SOURCES})
target_link_libraries(data_structures PRIVATE common_flags)
target_link_libraries(gamesman PRIVATE data_structures)

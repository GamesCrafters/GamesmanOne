set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/bitstream.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cstring.h
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_hash_map_ext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_hash_map.h
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_queue.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/bitstream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cstring.c
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_array.c
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_hash_map_ext.c
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_hash_map.c
    ${CMAKE_CURRENT_SOURCE_DIR}/int64_queue.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

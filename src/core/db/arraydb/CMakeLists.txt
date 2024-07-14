set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/arraydb.h
    ${CMAKE_CURRENT_SOURCE_DIR}/record_array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/record.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/arraydb.c
    ${CMAKE_CURRENT_SOURCE_DIR}/record_array.c
    ${CMAKE_CURRENT_SOURCE_DIR}/record.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

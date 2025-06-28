set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/arraydb.h
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_record_array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_record.h
    ${CMAKE_CURRENT_SOURCE_DIR}/record_array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/record.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/arraydb.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

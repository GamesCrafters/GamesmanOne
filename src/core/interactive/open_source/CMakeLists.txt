set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/open_source.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/open_source.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

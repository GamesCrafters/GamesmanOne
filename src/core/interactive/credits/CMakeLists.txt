set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/credits.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/credits.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

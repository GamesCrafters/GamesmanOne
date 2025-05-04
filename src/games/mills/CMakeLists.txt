set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/board_formats.h
    ${CMAKE_CURRENT_SOURCE_DIR}/masks.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mills.h
    ${CMAKE_CURRENT_SOURCE_DIR}/variants.h
)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/mills.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/dshogi.h
            ${CMAKE_CURRENT_SOURCE_DIR}/dshogi_constants.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/dshogi.c
            ${CMAKE_CURRENT_SOURCE_DIR}/dshogi_constants.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

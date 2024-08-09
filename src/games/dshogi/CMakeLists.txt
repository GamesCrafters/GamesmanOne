set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/dshogi.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/dshogi.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

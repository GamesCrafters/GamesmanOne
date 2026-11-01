set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/winkers.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/winkers.c)

target_sources(gamesman_core PRIVATE ${HEADERS} ${SOURCES})

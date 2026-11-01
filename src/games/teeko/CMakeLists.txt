set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/teeko.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/teeko.c)

target_sources(gamesman_core PRIVATE ${HEADERS} ${SOURCES})

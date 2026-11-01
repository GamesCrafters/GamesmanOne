set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/mallqueenschess.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/mallqueenschess.c)

target_sources(gamesman_core PRIVATE ${HEADERS} ${SOURCES})

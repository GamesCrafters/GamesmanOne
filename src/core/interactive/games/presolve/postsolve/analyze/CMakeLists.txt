set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/analyze.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/analyze.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

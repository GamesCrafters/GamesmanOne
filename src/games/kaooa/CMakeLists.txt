set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/kaooa.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/kaooa.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

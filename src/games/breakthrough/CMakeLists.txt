set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/breakthrough.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/breakthrough.c)

target_sources(gamesman_core PRIVATE ${HEADERS} ${SOURCES})

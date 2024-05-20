set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/choices.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/choices.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

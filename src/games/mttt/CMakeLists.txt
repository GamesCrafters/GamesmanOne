set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/mttt.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/mttt.c)

target_sources(gamesman_core PRIVATE ${HEADERS} ${SOURCES})

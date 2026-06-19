set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/naivedb.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/naivedb.c)

target_sources(gamesman_core PRIVATE ${HEADERS} ${SOURCES})

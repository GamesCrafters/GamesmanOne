set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/configure.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/configure.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

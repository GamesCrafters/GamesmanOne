set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/mtttier.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/mtttier.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

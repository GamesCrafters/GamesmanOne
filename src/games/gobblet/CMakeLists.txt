set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/gobblet_gobblers.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/gobblet_gobblers.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

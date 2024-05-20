set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/game_help.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/game_help.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

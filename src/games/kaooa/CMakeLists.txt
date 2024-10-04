set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/quixo.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/quixo.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

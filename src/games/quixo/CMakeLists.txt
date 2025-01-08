set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/quixo.h
            ${CMAKE_CURRENT_SOURCE_DIR}/quixo_constants.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/quixo.c
            ${CMAKE_CURRENT_SOURCE_DIR}/quixo_constants.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

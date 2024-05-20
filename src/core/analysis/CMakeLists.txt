set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/analysis.h
            ${CMAKE_CURRENT_SOURCE_DIR}/stat_manager.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/analysis.c
            ${CMAKE_CURRENT_SOURCE_DIR}/stat_manager.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

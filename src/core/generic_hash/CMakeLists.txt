set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/context.h
            ${CMAKE_CURRENT_SOURCE_DIR}/generic_hash.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/context.c
            ${CMAKE_CURRENT_SOURCE_DIR}/generic_hash.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

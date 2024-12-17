set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/gates_tier.h
            ${CMAKE_CURRENT_SOURCE_DIR}/gates.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/gates_tier.c
            ${CMAKE_CURRENT_SOURCE_DIR}/gates.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/frontier_percolation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/frontier.h
    ${CMAKE_CURRENT_SOURCE_DIR}/frontierless.h
    ${CMAKE_CURRENT_SOURCE_DIR}/one_bit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/reverse_graph.h
    ${CMAKE_CURRENT_SOURCE_DIR}/types.h
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/frontier_percolation.c
    ${CMAKE_CURRENT_SOURCE_DIR}/frontier.c
    ${CMAKE_CURRENT_SOURCE_DIR}/frontierless.c
    ${CMAKE_CURRENT_SOURCE_DIR}/reverse_graph.c
)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

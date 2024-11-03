set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/bi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/frontier.h
    ${CMAKE_CURRENT_SOURCE_DIR}/reverse_graph.h
    ${CMAKE_CURRENT_SOURCE_DIR}/test.h
    ${CMAKE_CURRENT_SOURCE_DIR}/vi.h
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/bi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/frontier.c
    ${CMAKE_CURRENT_SOURCE_DIR}/reverse_graph.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/vi.c
)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

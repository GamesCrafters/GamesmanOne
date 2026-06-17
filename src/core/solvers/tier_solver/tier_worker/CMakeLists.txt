add_subdirectory(backward_induction)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/bi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/it.h
    ${CMAKE_CURRENT_SOURCE_DIR}/test.h
    ${CMAKE_CURRENT_SOURCE_DIR}/vi.h
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/bi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/vi.c
)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

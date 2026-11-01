set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/savio.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scriptgen.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/savio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/scriptgen.c)

target_sources(gamesman_core PRIVATE ${HEADERS} ${SOURCES})

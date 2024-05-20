set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/solver_options.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/solver_options.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

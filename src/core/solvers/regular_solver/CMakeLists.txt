set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/regular_solver.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/regular_solver.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

# Add subdirectories for each component
add_subdirectory(options)
add_subdirectory(postsolve)
add_subdirectory(savio)
add_subdirectory(solver_options)

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/match.h
    ${CMAKE_CURRENT_SOURCE_DIR}/presolve.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/match.c
    ${CMAKE_CURRENT_SOURCE_DIR}/presolve.c)

target_sources(gamesman_core PRIVATE ${HEADERS} ${SOURCES})

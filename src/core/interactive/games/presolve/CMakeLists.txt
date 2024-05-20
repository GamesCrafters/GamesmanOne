# Add subdirectories for each component
add_subdirectory(options)
add_subdirectory(postsolve)
add_subdirectory(savio)
add_subdirectory(solver_options)

set(LIBS
    core_interactive_games_presolve_options
    core_interactive_games_presolve_postsolve
    core_interactive_games_presolve_savio
    core_interactive_games_presolve_solver_options)

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/match.h
            ${CMAKE_CURRENT_SOURCE_DIR}/presolve.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/match.c
            ${CMAKE_CURRENT_SOURCE_DIR}/presolve.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

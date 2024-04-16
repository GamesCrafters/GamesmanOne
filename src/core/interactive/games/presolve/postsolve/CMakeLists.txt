# Add subdirectories for each component
add_subdirectory(analyze)
add_subdirectory(configure)
add_subdirectory(help)
add_subdirectory(play)

set(LIBS
    core_interactive_games_presolve_postsolve_analyze
    core_interactive_games_presolve_postsolve_configure
    core_interactive_games_presolve_postsolve_help
    core_interactive_games_presolve_postsolve_play)

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/postsolve.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/postsolve.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

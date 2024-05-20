# Add subdirectories for each component
add_subdirectory(choices)

set(LIBS core_interactive_games_presolve_options_choices)

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/options.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/options.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

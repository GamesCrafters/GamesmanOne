# Add subdirectories for each component
add_subdirectory(credits)
add_subdirectory(games)
add_subdirectory(help)

set(LIBS core_interactive_credits core_interactive_games core_interactive_help)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/automenu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/main_menu.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/automenu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/main_menu.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

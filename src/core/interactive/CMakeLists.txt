# Add subdirectories for each component
add_subdirectory(open_source)
add_subdirectory(games)
add_subdirectory(help)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/automenu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/main_menu.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/automenu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/main_menu.c)

target_sources(gamesman_core PRIVATE ${HEADERS} ${SOURCES})

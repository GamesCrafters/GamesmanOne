# Add subdirectories for each component
add_subdirectory(presolve)

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/games.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/games.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

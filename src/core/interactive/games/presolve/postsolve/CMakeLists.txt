# Add subdirectories for each component
add_subdirectory(analyze)
add_subdirectory(configure)
add_subdirectory(help)
add_subdirectory(play)

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/postsolve.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/postsolve.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

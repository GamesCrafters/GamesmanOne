# Add subdirectories for each component
add_subdirectory(choices)

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/options.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/options.c)

target_sources(gamesman_core PRIVATE ${HEADERS} ${SOURCES})

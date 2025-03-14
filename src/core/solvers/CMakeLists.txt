# Add subdirectories for each component
add_subdirectory(regular_solver)
add_subdirectory(tier_solver)

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/solver_manager.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/solver_manager.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

# Add subdirectories for each component
add_subdirectory(analysis)
add_subdirectory(data_structures)
add_subdirectory(db)
add_subdirectory(generic_hash)
add_subdirectory(headless)
add_subdirectory(interactive)
add_subdirectory(savio)
add_subdirectory(solvers)
add_subdirectory(types)

set(LIBS
    core_analysis
    core_data_structures
    core_db
    core_generic_hash
    core_headless
    core_interactive
    core_savio
    core_solvers
    core_types)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/constants.h
    ${CMAKE_CURRENT_SOURCE_DIR}/game_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gamesman_headless.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gamesman_interactive.h
    ${CMAKE_CURRENT_SOURCE_DIR}/misc.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/constants.c
    ${CMAKE_CURRENT_SOURCE_DIR}/game_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gamesman_headless.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gamesman_interactive.c
    ${CMAKE_CURRENT_SOURCE_DIR}/misc.c)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

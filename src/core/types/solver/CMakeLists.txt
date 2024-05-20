set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/solver_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/solver_option.h
    ${CMAKE_CURRENT_SOURCE_DIR}/solver.h)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

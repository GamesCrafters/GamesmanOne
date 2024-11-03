add_subdirectory(tier_worker)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/reverse_tier_graph.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_analyzer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_solver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_worker.h
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/reverse_tier_graph.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_analyzer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_solver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_worker.c
)

target_sources(gamesman PRIVATE ${HEADERS} ${SOURCES})

set(MPI_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_mpi.h
)

set(MPI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tier_mpi.c
)

if(USE_MPI)
    target_sources(gamesman PRIVATE ${MPI_HEADERS} ${MPI_SOURCES})
endif()

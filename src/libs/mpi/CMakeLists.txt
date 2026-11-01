set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/xmpi.h)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/xmpi.c)

add_library(gamesman_mpi STATIC ${HEADERS} ${SOURCES})
target_link_libraries(gamesman_mpi PRIVATE common_flags)
target_link_libraries(gamesman_mpi PRIVATE MPI::MPI_C)

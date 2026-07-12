# gamesman_memory library
add_library(gamesman_memory STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/concurrency.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gamesman_memory.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gamesman_memory.c)
target_link_libraries(gamesman_memory PRIVATE common_flags)
target_link_libraries(gamesman_memory PRIVATE lzma_wrapper)
if(OpenMP_FOUND) # OpenMP (optional)
  target_link_libraries(gamesman_memory PRIVATE OpenMP::OpenMP_C)
endif()

# misc library
add_library(misc STATIC ${CMAKE_CURRENT_SOURCE_DIR}/misc.h ${CMAKE_CURRENT_SOURCE_DIR}/misc.c)
if(USE_MPI) # MPI
  find_package(MPI REQUIRED)
  target_compile_definitions(misc PRIVATE USE_MPI)
  target_link_libraries(misc PRIVATE MPI::MPI_C)
endif()
target_link_libraries(misc PRIVATE common_flags)
find_package(ZLIB REQUIRED)
target_link_libraries(misc PUBLIC ZLIB::ZLIB)

# Add subdirectories for each component
add_subdirectory(analysis)
add_subdirectory(data_structures)
add_subdirectory(db)
add_subdirectory(hash)
add_subdirectory(headless)
add_subdirectory(interactive)
add_subdirectory(savio)
add_subdirectory(solvers)
add_subdirectory(types)

set(HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/concurrency.h
  ${CMAKE_CURRENT_SOURCE_DIR}/constants.h
  ${CMAKE_CURRENT_SOURCE_DIR}/game_manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gamesman_headless.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gamesman_interactive.h)

set(SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/constants.c
  ${CMAKE_CURRENT_SOURCE_DIR}/game_manager.c
  ${CMAKE_CURRENT_SOURCE_DIR}/gamesman_headless.c
  ${CMAKE_CURRENT_SOURCE_DIR}/gamesman_interactive.c)

target_sources(gamesman_core PRIVATE ${HEADERS} ${SOURCES})
target_link_libraries(gamesman_core PRIVATE misc gamesman_memory)

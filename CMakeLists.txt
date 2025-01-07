cmake_minimum_required(VERSION 3.14)
project(GamesmanOne VERSION 1.1.2 DESCRIPTION 2024.12.27)

option(DISABLE_OPENMP "Disable OpenMP." OFF)
option(USE_MPI "Enable MPI." OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Add configure header.
configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_SOURCE_DIR}/src/config.h"
)

# Add include paths.
include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/res/include")

# Add source.
add_subdirectory(src)

enable_testing()
add_executable(test_int64_array tests/data_structures/test_int64_array.c)
target_link_libraries(test_int64_array PRIVATE data_structures)
add_test(NAME TestInt64Array COMMAND test_int64_array)

cmake_minimum_required(VERSION 3.13)
project(GamesmanOne)

option(DISABLE_OPENMP "Disable OpenMP." OFF)
option(USE_MPI "Enable MPI." OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Add the source directory
include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/res/include")
add_subdirectory(src)

cmake_minimum_required(VERSION 3.14)
project(GamesmanOne VERSION 1.0.3 DESCRIPTION 2024.08.22)

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

# Find the liblzma library
find_library(LIBLZMA_LIBRARY
    NAMES lzma
    HINTS /opt/homebrew/lib /usr/local/lib
    REQUIRED)

# Find the lzma.h header file
find_path(LIBLZMA_INCLUDE_DIR
    NAMES lzma.h
    HINTS /opt/homebrew/include /usr/local/include
    REQUIRED)

# Include the directory containing lzma.h
target_include_directories(gamesman PRIVATE ${LIBLZMA_INCLUDE_DIR})

# Link the liblzma library to your target
target_link_libraries(gamesman PRIVATE ${LIBLZMA_LIBRARY})

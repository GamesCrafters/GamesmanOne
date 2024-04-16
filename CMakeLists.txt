cmake_minimum_required(VERSION 3.13)
project(GamesmanOne)

find_package(ZLIB REQUIRED)

list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/res/lib64/cmake")
find_package(json-c REQUIRED CONFIG)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Add the source directory
include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/res/include")
add_subdirectory(src)

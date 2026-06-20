cmake_minimum_required(VERSION 3.22)
project(GamesmanOne VERSION 1.4.0 DESCRIPTION 2026.06.18)

# Add the 'cmake' directory to the module search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Necessary for conditional compilation and flag inclusion
include(CheckCCompilerFlag)

# Necessary for line coverage generation.
include(CTest)

#######################
# Compile-time Macros #
#######################

# Run a small cache line size detection program to configure
# GamesmanOne_CACHE_LINE_SIZE
try_run(
  CACHE_LINE_RUN_RESULT
  CACHE_LINE_COMPILE_RESULT
  ${CMAKE_BINARY_DIR}/scripts
  ${PROJECT_SOURCE_DIR}/scripts/cacheline.c
  RUN_OUTPUT_STDOUT_VARIABLE CACHE_LINE_STDOUT
)
if(CACHE_LINE_COMPILE_RESULT AND CACHE_LINE_RUN_RESULT EQUAL 0)
  message(STATUS "Automatically detected CPU cache line size: ${CACHE_LINE_STDOUT} bytes.")
  set(GamesmanOne_CACHE_LINE_SIZE ${CACHE_LINE_STDOUT})
else()
  message(WARNING "Failed to automatically detect cache line size, falling back to 64 bytes.")
  set(GamesmanOne_CACHE_LINE_SIZE 64) # fallback
endif()

#####################################
# Global Compilation Configurations #
#####################################

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json in the build directory
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

########################
# Library Dependencies #
########################

include(gamesman_dependencies)

###########################################
# Compilation Configurations for Gamesman #
###########################################

# Set binary output directory to bin.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Add configure header.
configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_SOURCE_DIR}/src/config.h"
)

# Add --coverage flag to Gamesman targets when build type is Debug.
add_compile_options($<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:--coverage>)
add_link_options($<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:--coverage>)

########################
# Custom CMake Options #
########################

option(DISABLE_OPENMP "Disable OpenMP." OFF) # Set this to ON to disable OpenMP.
option(USE_MPI "Enable MPI." OFF) # Set this to ON to enable MPI.

##########################
# Common Flags Interface #
##########################

# Create an interface library for common flags
# Use the following command to link the flags to a target:
# target_link_libraries(<target> PRIVATE common_flags)
add_library(common_flags INTERFACE)
target_compile_options(common_flags INTERFACE
  # MSVC flags
  $<$<C_COMPILER_ID:MSVC>:/W4 /WX>

  # Non-MSVC flags
  $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -march=native>
)

#####################
# Ccache (optional) #
#####################

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
else()
  message(STATUS "ccache not found. Repeated compilation on unmodified targets may take longer.")
endif()

##################################
# Line Coverage (lcov, optional) #
##################################

find_program(LCOV_PATH lcov)
if(LCOV_PATH)
  message(STATUS "Found lcov: ${LCOV_PATH}")
  include(gamesman_lcov)
else()
  message(STATUS "lcov command not found. Code coverage targets will not be available.")
endif()

###################################
# include-what-you-use (optional) #
###################################

find_program(IWYU_PATH NAMES include-what-you-use iwyu)
if(IWYU_PATH)
  message(STATUS "Found IWYU: ${IWYU_PATH}")
  # Run IWYU alongside the C compiler, silencing Clang complaints about GCC flags
  set(CMAKE_C_INCLUDE_WHAT_YOU_USE
    ${IWYU_PATH}
    "-Wno-unknown-warning-option"
    "-Wno-ignored-optimization-argument"
  )
else()
  message(WARNING "Include What You Use not found. IWYU features will be disabled.")
endif()

##############################
# Static Analyzer (optional) #
##############################

# Enable static analyzer if possible when build type is "Debug"
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  include(gamesman_static_analyzer)
endif()

# Add include paths.
include_directories("${PROJECT_SOURCE_DIR}/src")

# Add source.
add_subdirectory(src)

# Add tests.
enable_testing()
add_subdirectory(tests)

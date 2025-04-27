cmake_minimum_required(VERSION 3.14)
project(GamesmanOne VERSION 1.3.0 DESCRIPTION 2025.04.26)

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

# Enable static analyzer if possible when build type is "Debug"
include(CheckCCompilerFlag)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")

  # GCC
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # Check if the C compiler supports -fanalyzer
    check_c_compiler_flag(-fanalyzer HAS_C_ANALYZER_FLAG)
    # Add the -fanalyzer flag if supported
    if(HAS_C_ANALYZER_FLAG)
      target_compile_options(common_flags INTERFACE $<$<COMPILE_LANGUAGE:C>:-fanalyzer>)
    endif()
  endif()

  # Clang Compiler
  if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang")
    # Check if the compiler supports static analyzer flags
    # Clang's static analyzer is typically invoked via scan-build, but we can use analyzer flags
    check_c_compiler_flag(-fsyntax-only HAS_C_ANALYZER_FLAG)
    # Add the flags if supported
    if(HAS_C_ANALYZER_FLAG)
      target_compile_options(common_flags INTERFACE $<$<COMPILE_LANGUAGE:C>:-fsyntax-only -Wunused -Wuninitialized>)
    endif()
  endif()

  # MSVC Compiler
  if(MSVC)
    # For MSVC, enable code analysis with /analyze flag
    check_c_compiler_flag("/analyze" HAS_C_ANALYZER_FLAG)
    if(HAS_C_ANALYZER_FLAG)
      target_compile_options(common_flags INTERFACE $<$<COMPILE_LANGUAGE:C>:/analyze>)
    endif()
  endif()

  # Add other compilers and flags as necessary
endif()

########################
# Custom CMake Options #
########################

option(DISABLE_OPENMP "Disable OpenMP." OFF) # Set this to ON to disable OpenMP.
option(USE_MPI "Enable MPI." OFF) # Set this to ON to enable MPI.

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
  set(GamesmanOne_CACHE_LINE_SIZE ${CACHE_LINE_STDOUT})
else()
  message(WARNING "Failed to automatically detect cache line size, falling back to 64 bytes.")
  set(GamesmanOne_CACHE_LINE_SIZE 64) # fallback
endif()

#####################################
# Global Compilation Configurations #
#####################################

# Set binary output directory to bin.
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

# Add tests.
enable_testing()
add_subdirectory(tests)

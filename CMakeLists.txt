##################
# Initialization #
##################

cmake_minimum_required(VERSION 3.25)

# Add the 'cmake' directory to the module search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# macOS Homebrew LLVM & OpenMP Setup
# Must execute BEFORE project() to correctly set compilers
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  include(gamesman_macos_llvm_openmp)
endif()

project(GamesmanOne VERSION 1.4.0 DESCRIPTION 2026.06.18)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

########################
# Options and Settings #
########################

option(GAMESMAN_ENABLE_OPENMP "Enable OpenMP." ON) # Set this to OFF to disable OpenMP
option(GAMESMAN_ENABLE_MPI "Enable MPI." OFF) # Set this to ON to enable MPI
option(GAMESMAN_ENABLE_STATIC_ANALYZER "Enable compiler static analyzer." OFF) # Set this to ON to enable compile-time static code analysis
option(GAMESMAN_ENABLE_ASAN_UBSAN "Enable address sanitizer and undefined behavior sanitizer." OFF) # Set this to ON to enable ASan and UBSan
option(GAMESMAN_ENABLE_TSAN "Enable thread sanitizer." OFF) # Set this to ON to enable TSan
option(GAMESMAN_ENABLE_LINE_COVERAGE "Enable line coverage." OFF) # Set this to ON to enable line coverage
option(GAMESMAN_ENABLE_BENCHMARKS "Build targets for benchmarking." OFF) # Set this to ON to enable benchmarks
option(GAMESMAN_ENABLE_TESTING "Build test targets." ON) # Set this to OFF to disable testing
option(GAMESMAN_ENABLE_ANIMATION "Enable animations in interactive mode." OFF) # Set this to ON to enable animations

if(GAMESMAN_ENABLE_ASAN_UBSAN)
  message(STATUS "Sanitizers enabled: ASan, LSan UBSan")
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

if(GAMESMAN_ENABLE_TSAN)
  message(STATUS "Sanitizers enabled: TSan")
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()

########################
# Library Dependencies #
########################

# ccache (optional)
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
else()
  message(STATUS "ccache not found. Recompilation of unmodified targets may take longer.")
endif()

include(gamesman_dependencies)

# Necessary for conditional compilation and flag inclusion
include(CheckCCompilerFlag)

#######################################
# Common Flags Interface for Gamesman #
#######################################

# common_flags will be linked to all Gamesman targets
add_library(common_flags INTERFACE)
target_compile_options(common_flags INTERFACE -Wall -Wextra -Wpedantic -march=native)

if(GAMESMAN_ENABLE_STATIC_ANALYZER)
  include(gamesman_static_analyzer) # Modifies common_flags internally
endif()

if(GAMESMAN_ENABLE_LINE_COVERAGE)
  find_program(LCOV_PATH lcov)
  if(LCOV_PATH)
    message(STATUS "Found lcov: ${LCOV_PATH}")
    target_compile_options(common_flags INTERFACE --coverage)
    target_link_options(common_flags INTERFACE --coverage)
    include(gamesman_lcov)
  else()
    message(STATUS "lcov not found. Code coverage targets will not be available.")
  endif()
endif()

check_c_compiler_flag("-msse2" GAMESMAN_HAS_SSE2)
if(GAMESMAN_HAS_SSE2)
  target_compile_definitions(common_flags INTERFACE GAMESMAN_HAS_SSE2)
endif()
check_c_compiler_flag("-mssse3" GAMESMAN_HAS_SSSE3)
if(GAMESMAN_HAS_SSSE3)
  target_compile_definitions(common_flags INTERFACE GAMESMAN_HAS_SSSE3)
endif()
check_c_compiler_flag("-msse4.1" GAMESMAN_HAS_SSE4_1)
if(GAMESMAN_HAS_SSE4_1)
  target_compile_definitions(common_flags INTERFACE GAMESMAN_HAS_SSE4_1)
endif()
check_c_compiler_flag("-mbmi" GAMESMAN_HAS_BMI1)
if(GAMESMAN_HAS_BMI1)
  target_compile_definitions(common_flags INTERFACE GAMESMAN_HAS_BMI1)
endif()
check_c_compiler_flag("-mbmi2" GAMESMAN_HAS_BMI2)
if(GAMESMAN_HAS_BMI2)
  target_compile_definitions(common_flags INTERFACE GAMESMAN_HAS_BMI2)
endif()

########################
# Macros/System Checks #
########################

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

# Detect system processor count
include(ProcessorCount)
ProcessorCount(GamesmanOne_NUM_CPUS)
if(GamesmanOne_NUM_CPUS GREATER 0)
  message(STATUS "Automatically detected processor count: ${GamesmanOne_NUM_CPUS}.")
else()
  message(WARNING "Failed to automatically detect processor count, falling back to 1.")
  set(GamesmanOne_NUM_CPUS 1)
endif()

# Check and enable link-time optimizations for "Release" builds if possible.
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_support_error)

if(ipo_supported)
  message(STATUS "Found IPO / LTO, will enable for Release bulids (current config: ${CMAKE_BUILD_TYPE})")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
else()
  message(STATUS "IPO / LTO not supported: <${ipo_support_error}>")
endif()

# Add configure header.
configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_SOURCE_DIR}/src/config.h"
)

####################
# Gamesman Targets #
####################

# Add include paths.
include_directories("${PROJECT_SOURCE_DIR}/src")

# Add source.
add_subdirectory(src)

if(GAMESMAN_ENABLE_TESTING)
  # Necessary for line coverage generation. Calls enable_testing() internally.
  include(CTest)
  add_subdirectory(tests)
endif()

# Benchmarks
if(GAMESMAN_ENABLE_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

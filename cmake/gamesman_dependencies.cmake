include(FetchContent)

# Temporarily disable tests so that the tests from the following dependencies
# do not get invoked with our own tests.
set(BUILD_TESTING OFF CACHE BOOL "Temporarily disable tests" FORCE)

######################################### Google Benchmark #########################################

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG 192ef10025eb2c4cdd392bc502f0c852196baa48 # v1.9.5
)

# Disable Google Benchmark's own internal tests and installation to save build time
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark install" FORCE)

############################################ GoogleTest ############################################

# GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723 # v1.17.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

############################################## json-c ##############################################

FetchContent_Declare(
    json-c
    GIT_REPOSITORY https://github.com/json-c/json-c.git
    GIT_TAG 05180c1fa385eb39274cf0a17223da1b82832773 # 0.18
)

############################################# XZ Utils #############################################

# Suppress liblzma native language support warning
# Disables NLS translations for xz
set(XZ_NLS OFF CACHE BOOL "Disable Native Language Support for xz" FORCE)

FetchContent_Declare(
    liblzma
    GIT_REPOSITORY https://github.com/tukaani-project/xz.git
    GIT_TAG ebb0e6789cefe3be71756881aa8f2009fda9938c # 5.8.3
)

################################################ LZ4 ###############################################

FetchContent_Declare(
    lz4
    GIT_REPOSITORY https://github.com/lz4/lz4.git
    GIT_TAG ebb370ca83af193212df4dcbadcc5d87bc0de2f0 # v1.10.0
    SOURCE_SUBDIR build/cmake
)

####################################################################################################

# Pull the above dependencies from their sources.
FetchContent_MakeAvailable(json-c liblzma lz4)
if(GAMESMAN_ENABLE_BENCHMARKS)
    FetchContent_MakeAvailable(googlebenchmark)
endif()
if(GAMESMAN_ENABLE_TESTING)
    FetchContent_MakeAvailable(googletest)
endif()

# Re-enable testing for GAMESMAN tests.
set(BUILD_TESTING ON CACHE BOOL "Restore testing" FORCE)

######################################## XZ Utils (wrapper) ########################################

add_library(lzma_wrapper INTERFACE)
target_link_libraries(lzma_wrapper INTERFACE liblzma)
target_include_directories(lzma_wrapper INTERFACE
    "${liblzma_SOURCE_DIR}/src/liblzma/api"
)

############################################## OpenMP ##############################################

if(GAMESMAN_ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_FOUND)
        message(STATUS "OpenMP multithreading enabled")
    else()
        message(WARNING "OpenMP not found, configuring without multithreading")
    endif()
else()
    message(STATUS "OpenMP multithreading disabled")
endif()

################################################ MPI ###############################################

if(GAMESMAN_ENABLE_MPI)
    find_package(MPI REQUIRED)
    message(STATUS "MPI enabled")
endif()

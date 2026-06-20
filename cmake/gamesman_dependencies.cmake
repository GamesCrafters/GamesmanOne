include(FetchContent)

# Temporarily disable tests so that the tests from the following dependencies
# do not get invoked with our own tests.
set(BUILD_TESTING OFF CACHE BOOL "Temporarily disable tests" FORCE)

# GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723 # v1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# json-c
FetchContent_Declare(
    json-c
    GIT_REPOSITORY https://github.com/json-c/json-c.git
    GIT_TAG 05180c1fa385eb39274cf0a17223da1b82832773 # 0.18
)

# XZ Utils
FetchContent_Declare(
    liblzma
    GIT_REPOSITORY https://github.com/tukaani-project/xz.git
    GIT_TAG ebb0e6789cefe3be71756881aa8f2009fda9938c # 5.8.3
)

# LZ4
FetchContent_Declare(
    lz4
    GIT_REPOSITORY https://github.com/lz4/lz4.git
    GIT_TAG ebb370ca83af193212df4dcbadcc5d87bc0de2f0 # v1.10.0
)

# Pull the above dependencies from their sources.
FetchContent_MakeAvailable(googletest json-c liblzma lz4)

# Re-enable testing for our own tests.
set(BUILD_TESTING ON CACHE BOOL "Restore testing" FORCE)

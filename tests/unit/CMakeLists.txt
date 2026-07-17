# ==============================================================================
# add_library_test
#
# Generates a GoogleTest executable for a specific internal library, links 
# required dependencies (including GoogleTest), and registers the discovered 
# tests with CTest.
#
# Usage:
#   add_library_test(<target_name> <library_to_test>
#                    SOURCES <source1> [<source2> ...]
#                    [EXTRA_LIBS <lib1> ...]
#                    [COMPILE_DEFINITIONS <def1> ...]
#                    [LABELS <label1> ...]
#                    [TIMEOUT <seconds>])
#
# Positional Arguments:
#   <target_name>       - The name of the CMake executable target to create.
#   <library_to_test>   - The name of the internal library target being tested. 
#                         This is automatically linked to the test executable.
#
# Keyword Arguments:
#   SOURCES             - (Required) A list of C++ source files containing the tests.
#   EXTRA_LIBS          - (Optional) Additional dependencies to link against the 
#                         test executable (e.g., third-party libraries, mocks).
#   COMPILE_DEFINITIONS - (Optional) A list of preprocessor definitions to apply 
#                         privately to the generated test executable.
#   LABELS              - (Optional) A list of CTest labels applied to all discovered 
#                         tests for filtering. If omitted, defaults to "unit" and 
#                         the name of the <library_to_test>.
#   TIMEOUT             - (Optional) A timeout value in seconds applied to the tests.
#
# Example:
#   add_library_test(NetworkLibTests NetworkLib
#       SOURCES 
#           SocketTest.cpp
#           HttpParserTest.cpp
#       EXTRA_LIBS
#           OpenSSL::SSL
#       COMPILE_DEFINITIONS
#           ENABLE_TEST_LOGGING=1
#           MOCK_NETWORK_LATENCY=50
#       LABELS 
#           "integration" "network"
#       TIMEOUT 
#           120
#   )
# ==============================================================================
function(add_library_test target_name library_to_test)
    # 1. Define the named arguments we want to accept
    set(options "") # True/False flags (e.g., NO_DEFAULT_LABELS)
    set(oneValueArgs TIMEOUT) # Arguments that take a single value
    set(multiValueArgs SOURCES EXTRA_LIBS LABELS COMPILE_DEFINITIONS) # Arguments that take lists

    # 2. Parse the arguments passed to the function
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Ensure sources were actually provided
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "add_library_test requires at least one file in SOURCES")
    endif()

    # 3. Create the executable
    add_executable(${target_name} ${ARG_SOURCES})

    # Delete previous profile data if line coverage is enabled
    if(ENABLE_LINE_COVERAGE)
        add_dependencies(${target_name} coverage-delete)
    endif()

    # Apply custom compile definitions if provided
    if(ARG_COMPILE_DEFINITIONS)
        target_compile_definitions(${target_name} PRIVATE ${ARG_COMPILE_DEFINITIONS})
    endif()

    # 4. Link dependencies (Target Library + GTest + any extras)
    target_link_libraries(${target_name}
        PRIVATE
        common_flags
        ${library_to_test}
        GTest::gtest_main
        GTest::gmock
        ${ARG_EXTRA_LIBS} # Link additional libs if provided
    )

    # 5. Dynamically build the CTest properties list
    set(TEST_PROPERTIES "")

    # Apply custom labels if provided, otherwise default to "unit" and the library name
    if(ARG_LABELS)
        foreach(label IN LISTS ARG_LABELS)
            list(APPEND TEST_PROPERTIES LABELS "${label}")
        endforeach()
    else()
        list(APPEND TEST_PROPERTIES LABELS "unit" LABELS "${library_to_test}")
    endif()

    # Apply timeout if provided
    if(ARG_TIMEOUT)
        list(APPEND TEST_PROPERTIES TIMEOUT ${ARG_TIMEOUT})
    endif()

    # 6. Discover tests and apply the dynamically built properties
    include(GoogleTest)
    gtest_discover_tests(${target_name}
        PROPERTIES ${TEST_PROPERTIES}
    )
endfunction()

# This test executable contains test cases for all source files directly linked
# to gamesman_core without wrapper libraries. Since its source files are
# scattered in subdirectories, we cannot use the add_library_test function here.
add_executable(gamesman_test)

target_link_libraries(gamesman_test PRIVATE
    common_flags
    gamesman_core
    GTest::gtest_main
)

add_subdirectory(core)
add_subdirectory(libs)

# Discover all tests for gamesman_test
include(GoogleTest)
gtest_discover_tests(gamesman_test
    PROPERTIES LABELS "unit" "gamesman_core"
)

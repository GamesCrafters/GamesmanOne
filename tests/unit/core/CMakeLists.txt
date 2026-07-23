add_subdirectory(data_structures)
add_library_test(misc_test misc
    SOURCES
    misc_test.cpp
)

# Tests for concurrency.h
add_executable(gamesman_concurrency_test concurrency_test.cpp)
target_link_libraries(gamesman_concurrency_test PRIVATE
    common_flags
    GTest::gtest_main
)
if(OpenMP_FOUND)
    target_link_libraries(gamesman_concurrency_test PRIVATE
        OpenMP::OpenMP_CXX
    )
endif()
include(GoogleTest)
gtest_add_tests(
    TARGET gamesman_concurrency_test
    EXTRA_ARGS --gtest_catch_exceptions=0
    TEST_LIST discovered_tests
)

set_tests_properties(${discovered_tests}
    PROPERTIES LABELS "unit;gamesman_concurrency"
)

target_compile_definitions(gamesman_concurrency_test PRIVATE GM_NUM_CPUS=${GamesmanOne_NUM_CPUS})

# Apply the PROCESSORS property exclusively to the multithreaded tests
set_tests_properties(
    ConcurrentBoolTest.MultithreadedVisibility
    ConcurrentBoolTest.IdempotentConcurrentWrites
    ConcurrentIntTest.MultithreadedMaxCalculation
    ConcurrentInt64Test.IdempotentConcurrentWrites
    ConcurrentSizeTypeTest.MultithreadedArithmeticConsistency
    ConcurrentSizeTypeTest.MultithreadedResourceAllocation
    OpenMpThreadUtilitiesTest.UniqueThreadIdsInParallelRegion
    PROPERTIES PROCESSORS ${GamesmanOne_NUM_CPUS}
)

add_subdirectory(data_structures)

add_library_test(gamesman_memory_test gamesman_memory
    SOURCES
    gamesman_memory_test.cpp
    EXTRA_LIBS
    $<$<BOOL:${OpenMP_FOUND}>:OpenMP::OpenMP_CXX>
)

add_library_test(misc_test misc
    SOURCES
    misc_test.cpp
)

# Tests for concurrency.h
# TODO: move these tests to a different category and keep unit tests
# deterministic.
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

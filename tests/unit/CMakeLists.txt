add_executable(gamesman_test)

target_link_libraries(gamesman_test PRIVATE
    common_flags
    gamesman_core
    GTest::gtest_main
)

add_subdirectory(core)
add_subdirectory(libs)

# Discover all tests for CTest
include(GoogleTest)
gtest_discover_tests(gamesman_test
    PROPERTIES LABELS "unit"
)

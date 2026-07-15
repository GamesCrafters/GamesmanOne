set(LCOV_BASELINE_INFO "${CMAKE_BINARY_DIR}/baseline.info")
set(LCOV_TEST_INFO "${CMAKE_BINARY_DIR}/test.info")
set(LCOV_MERGED_INFO "${CMAKE_BINARY_DIR}/merged.info")
set(LCOV_OUTPUT_INFO "${CMAKE_BINARY_DIR}/lcov.info")
set(LCOV_SRC_FILTER "'/usr/*'" "build/*")

# For vscode CMake Tools line coverage configuration cmake.preRunCoverageTarget
# Wipes old coverage data before tests run
add_custom_target(coverage-clean
    # Wipe old execution data
    COMMAND ${LCOV_PATH} --branch-coverage -z -d ${CMAKE_BINARY_DIR}

    # Capture the baseline
    COMMAND ${LCOV_PATH} --branch-coverage -c -i -q -d ${CMAKE_BINARY_DIR} -o ${LCOV_BASELINE_INFO}
    --ignore-errors mismatch,mismatch
    --ignore-errors gcov,gcov
    --ignore-errors source,source
    --filter range

    COMMENT "Resetting counters and capturing 0% baseline..."
)

# For vscode CMake Tools line coverage configuration cmake.postRunCoverageTarget
# Captures new data and merges with baseline after tests run
add_custom_target(coverage-generate
    # Capture the actual test execution data
    COMMAND ${LCOV_PATH} --branch-coverage -q -c -d ${CMAKE_BINARY_DIR} -o ${LCOV_TEST_INFO} --ignore-errors mismatch,mismatch

    # Merge the 0% baseline with the actual test data
    COMMAND ${LCOV_PATH} --branch-coverage -a ${LCOV_BASELINE_INFO} -a ${LCOV_TEST_INFO} -o ${LCOV_MERGED_INFO}

    # Filter the merged file to remove system and test files 
    COMMAND ${LCOV_PATH} --branch-coverage --ignore-errors unused -q -r ${LCOV_MERGED_INFO} ${LCOV_SRC_FILTER} -o ${LCOV_OUTPUT_INFO}
    --ignore-errors mismatch,mismatch

    COMMENT "Generating, merging, and filtering coverage info..."
)

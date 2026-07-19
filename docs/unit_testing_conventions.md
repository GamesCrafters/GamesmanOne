# C/C++ Unit Testing Conventions (GoogleTest)

This document outlines the standard conventions for writing GoogleTest unit tests in this project. Adhering to these guidelines ensures our test suite remains reliable, readable, and safe for parallel execution.

## 1. Naming Conventions
GoogleTest has specific requirements for macro arguments to prevent compilation errors and ensure smooth integration with test runners.

* **PascalCase Only:** Both the Test Suite Name (first argument) and the Test Name (second argument) must be strictly in `PascalCase`.
* **No Underscores:** Do not use underscores (`_`) in either the test suite name or the test name. 

**Example:**
```cpp
// Good
TEST(HashSetTest, AddSingleElement) { ... }

// Bad
TEST(hash_set_test, add_single_element) { ... }
```

## 2. Test Isolation (Parallel Safety)
Tests are executed in parallel. It is critical that tests do not interfere with one another.

* **Localize State**: Initialize all required data structures and state variables completely within the test body.
* **No Shared State**: Avoid global variables, static variables, or shared singletons unless strictly managed by a thread-safe fixture.
* **I/O Safety**: If a test involves I/O (like file reading/writing), ensure it uses unique, isolated file paths (e.g., using temporary directories or unique file names based on the test name) so parallel tests don't overwrite each other's data.

## 3. Commenting Guidelines
Keep comments clean, purposeful, and non-redundant.

* **Test-Level Comments (The "What")**: Place a brief comment directly above the TEST() macro. This comment should summarize exactly what behavior or edge case the test is verifying.
* **Inline Comments (The "Why")**: Avoid commenting what the code is doing (the code should be self-documenting). Instead, use inline comments only to explain why a specific, non-obvious decision was made (e.g., why a specific magic number is used, or why a specific memory lane is populated).

**Example:**

```cpp
// Verifies that a fully zeroed vector is handled correctly without being treated as a sentinel value.
TEST(HashSetTest, ZeroVector) {
    // ...
    // Distribute the loop counter across both 64-bit lanes to guarantee uniqueness and stress test the hash distribution.
    __m128i key = _mm_set_epi64x(i, ~i); 
    // ...
}
```

## 4. Edge Case Coverage
Always aim to test the boundaries and limitations of the API's contract to ensure robustness across all scenarios.

* **Boundary Conditions**: Test upper and lower limits, such as maximum capacities, minimum allowed values, or numerical extremes. Verify how the system behaves when it reaches, slightly exceeds, or falls just short of these limits to catch off-by-one errors or infinite loops.
* **Zero, Null, and Empty States**: Ensure that zero-initialized data, empty strings, null pointers, or empty collections are handled gracefully without causing crashes, unhandled exceptions, or undefined behavior.
* **Invalid or Missing Data**: Query for non-existent elements, provide malformed inputs, or simulate missing dependencies. Confirm that the system fails safely and returns the correct error states, default values, or boolean flags.
* **Duplicates and Idempotency**: Verify the behavior when identical inputs or operations are submitted multiple times. Ensure this does not result in duplicated internal state, memory corruption, or unintended side effects.
* **Initialization and Lifecycle**: Confirm that newly initialized objects are structurally sound and in their correct default state before any operations occur, and that state behaves predictably throughout the object's entire lifecycle.

/**
 * @file int64_hash_map_test.cpp
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @brief Unit tests for Int64HashMap.
 *
 * @copyright GamesCrafters research and development group
 * SPDX-License-Identifier: GPL-3.0-or-later
 * (see root COPYING file or accompanying source file)
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <set>
#include <utility>
#include <vector>

extern "C" {
#include "core/data_structures/int64_hash_map.h"
}

// ============================= Int64HashMapInit =============================

TEST(Int64HashMapTest, InitializesWithStandardLoadFactor) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    EXPECT_TRUE(Int64HashMapSet(&map, 42, 100))
        << "The map should initialize correctly and accept a basic insertion "
           "with a standard 0.5 load factor.";

    Int64HashMapIterator it = Int64HashMapGet(&map, 42);
    EXPECT_TRUE(Int64HashMapIteratorIsValid(&it))
        << "The map should return a valid iterator for a key that was just "
           "inserted.";

    EXPECT_EQ(Int64HashMapIteratorValue(&it), 100)
        << "The map should return the correct value for the inserted key.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, ClampsLoadFactorBelowMinimum) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.01);

    EXPECT_TRUE(Int64HashMapSet(&map, 1, 10))
        << "The map should be functional despite a below-minimum load factor "
           "input.";

    EXPECT_TRUE(Int64HashMapSet(&map, 2, 20))
        << "The map should allow multiple unique insertions after clamping.";

    Int64HashMapIterator it = Int64HashMapGet(&map, 1);
    EXPECT_TRUE(Int64HashMapIteratorIsValid(&it))
        << "The map should find a previously inserted key after clamping.";

    EXPECT_EQ(Int64HashMapIteratorValue(&it), 10)
        << "The map should return the correct value for the clamped case.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, ClampsLoadFactorAboveMaximum) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.99);

    EXPECT_TRUE(Int64HashMapSet(&map, -999, 111))
        << "The map should be functional despite an above-maximum load factor "
           "input.";

    EXPECT_TRUE(Int64HashMapSet(&map, 999, 222))
        << "The map should allow multiple unique insertions after clamping.";

    EXPECT_TRUE(Int64HashMapContains(&map, -999))
        << "The map should find a previously inserted key after clamping.";

    EXPECT_TRUE(Int64HashMapContains(&map, 999))
        << "The map should find all previously inserted keys.";

    Int64HashMapDestroy(&map);
}

// ============================ Int64HashMapDestroy ============================

TEST(Int64HashMapTest, DestroysEmptyMap) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    // Should not crash or leak memory.
    SUCCEED();
    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, DestroysPopulatedMap) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    Int64HashMapSet(&map, 1, 10);
    Int64HashMapSet(&map, 2, 20);
    Int64HashMapSet(&map, 3, 30);

    // Should not crash or leak memory.
    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, AllowsReuseAfterDestruction) {
    Int64HashMap map;

    Int64HashMapInit(&map, 0.5);
    EXPECT_TRUE(Int64HashMapSet(&map, 50, 500))
        << "The map should accept an insertion during its first lifecycle.";
    Int64HashMapDestroy(&map);

    Int64HashMapInit(&map, 0.5);
    EXPECT_TRUE(Int64HashMapSet(&map, 50, 999))
        << "The map should allow inserting the same key after being destroyed "
           "and re-initialized.";

    Int64HashMapIterator it = Int64HashMapGet(&map, 50);
    EXPECT_TRUE(Int64HashMapIteratorIsValid(&it))
        << "The map should find the key inserted during the second lifecycle.";

    EXPECT_EQ(Int64HashMapIteratorValue(&it), 999)
        << "The map should return the value from the second lifecycle, not the "
           "first.";

    Int64HashMapDestroy(&map);
}

// ============================== Int64HashMapSet ==============================

TEST(Int64HashMapTest, SetsSingleEntry) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    EXPECT_TRUE(Int64HashMapSet(&map, 42, 100))
        << "Setting a single key-value pair should succeed.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, SetsMultipleDistinctEntries) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    EXPECT_TRUE(Int64HashMapSet(&map, 10, 100))
        << "Setting the first key-value pair should succeed.";
    EXPECT_TRUE(Int64HashMapSet(&map, 20, 200))
        << "Setting the second key-value pair should succeed.";
    EXPECT_TRUE(Int64HashMapSet(&map, 30, 300))
        << "Setting the third key-value pair should succeed.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, OverwritesExistingKeyWithNewValue) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    EXPECT_TRUE(Int64HashMapSet(&map, 42, 100))
        << "Initial insertion should succeed.";

    EXPECT_TRUE(Int64HashMapSet(&map, 42, 999))
        << "Overwriting an existing key with a new value should succeed.";

    Int64HashMapIterator it = Int64HashMapGet(&map, 42);
    EXPECT_TRUE(Int64HashMapIteratorIsValid(&it))
        << "The key should still be found after overwriting.";

    EXPECT_EQ(Int64HashMapIteratorValue(&it), 999)
        << "The value should be updated to the new value after overwriting.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, OverwriteMultipleTimesRetainsLatestValue) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    for (int64_t v = 0; v < 10; ++v) {
        EXPECT_TRUE(Int64HashMapSet(&map, 42, v))
            << "Overwrite #" << v << " should succeed.";
    }

    Int64HashMapIterator it = Int64HashMapGet(&map, 42);
    EXPECT_TRUE(Int64HashMapIteratorIsValid(&it))
        << "The key should still be found after repeated overwrites.";

    EXPECT_EQ(Int64HashMapIteratorValue(&it), 9)
        << "The value should reflect the last overwrite.";

    Int64HashMapDestroy(&map);
}

// ============================== Int64HashMapGet ==============================

TEST(Int64HashMapTest, GetReturnsValidIteratorForExistingKey) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    Int64HashMapSet(&map, 42, 100);

    Int64HashMapIterator it = Int64HashMapGet(&map, 42);
    EXPECT_TRUE(Int64HashMapIteratorIsValid(&it))
        << "Get should return a valid iterator for an existing key.";

    EXPECT_EQ(Int64HashMapIteratorKey(&it), 42)
        << "The iterator should point to the correct key.";

    EXPECT_EQ(Int64HashMapIteratorValue(&it), 100)
        << "The iterator should point to the correct value.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, GetReturnsInvalidIteratorForMissingKey) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    Int64HashMapSet(&map, 42, 100);

    Int64HashMapIterator it = Int64HashMapGet(&map, 99);
    EXPECT_FALSE(Int64HashMapIteratorIsValid(&it))
        << "Get should return an invalid iterator for a key that was never "
           "inserted.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, GetReturnsInvalidIteratorForEmptyMap) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    Int64HashMapIterator it = Int64HashMapGet(&map, 42);
    EXPECT_FALSE(Int64HashMapIteratorIsValid(&it))
        << "Get should return an invalid iterator when the map is empty.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, GetReturnsCorrectValuesForMultipleKeys) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    Int64HashMapSet(&map, 10, 100);
    Int64HashMapSet(&map, 20, 200);
    Int64HashMapSet(&map, 30, 300);

    for (int64_t key = 10; key <= 30; key += 10) {
        Int64HashMapIterator it = Int64HashMapGet(&map, key);
        EXPECT_TRUE(Int64HashMapIteratorIsValid(&it))
            << "Get should return a valid iterator for key " << key << ".";

        EXPECT_EQ(Int64HashMapIteratorKey(&it), key)
            << "The iterator should point to the correct key.";

        EXPECT_EQ(Int64HashMapIteratorValue(&it), key * 10)
            << "The iterator should point to the correct value for key " << key
            << ".";
    }

    Int64HashMapDestroy(&map);
}

// =========================== Int64HashMapContains ===========================

TEST(Int64HashMapTest, ContainsReturnsTrueForExistingKeys) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    Int64HashMapSet(&map, 10, 100);
    Int64HashMapSet(&map, 20, 200);

    EXPECT_TRUE(Int64HashMapContains(&map, 10))
        << "Contains should return true for the first inserted key.";

    EXPECT_TRUE(Int64HashMapContains(&map, 20))
        << "Contains should return true for the second inserted key.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, ContainsReturnsFalseForMissingKeys) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    Int64HashMapSet(&map, 10, 100);

    EXPECT_FALSE(Int64HashMapContains(&map, 20))
        << "Contains should return false for a key that was never inserted.";

    EXPECT_FALSE(Int64HashMapContains(&map, 0))
        << "Contains should return false for zero when it was never inserted.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, ContainsReturnsFalseForEmptyMap) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    EXPECT_FALSE(Int64HashMapContains(&map, 42))
        << "Contains should return false when the map is completely empty.";

    Int64HashMapDestroy(&map);
}

// ============================= Edge Case Values =============================

class Int64HashMapEdgeCaseTest
    : public ::testing::TestWithParam<std::pair<int64_t, int64_t>> {};

TEST_P(Int64HashMapEdgeCaseTest, HandlesEdgeCaseKeyValuePairs) {
    auto [key, value] = GetParam();

    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    EXPECT_FALSE(Int64HashMapContains(&map, key))
        << "The map should not contain key " << key << " when empty.";

    EXPECT_TRUE(Int64HashMapSet(&map, key, value))
        << "The map should successfully set key " << key << " to value "
        << value << ".";

    EXPECT_TRUE(Int64HashMapContains(&map, key))
        << "The map should contain key " << key << " after insertion.";

    Int64HashMapIterator it = Int64HashMapGet(&map, key);
    EXPECT_TRUE(Int64HashMapIteratorIsValid(&it))
        << "Get should return a valid iterator for key " << key << ".";

    EXPECT_EQ(Int64HashMapIteratorKey(&it), key)
        << "The iterator key should match.";

    EXPECT_EQ(Int64HashMapIteratorValue(&it), value)
        << "The iterator value should match.";

    Int64HashMapDestroy(&map);
}

INSTANTIATE_TEST_SUITE_P(
    EdgeCases, Int64HashMapEdgeCaseTest,
    ::testing::Values(
        // {key, value}
        std::make_pair(int64_t{0}, int64_t{0}),
        std::make_pair(int64_t{-1}, int64_t{-1}),
        std::make_pair(std::numeric_limits<int64_t>::max(),
                       std::numeric_limits<int64_t>::max()),
        std::make_pair(std::numeric_limits<int64_t>::max(),
                       std::numeric_limits<int64_t>::min()),
        std::make_pair(int64_t{0}, std::numeric_limits<int64_t>::max()),
        std::make_pair(int64_t{0}, std::numeric_limits<int64_t>::min()),
        std::make_pair(int64_t{1}, int64_t{0}),
        std::make_pair(int64_t{-1}, int64_t{0})));

// ================================= Iteration =================================

TEST(Int64HashMapTest, IteratesOverEmptyMap) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);
    Int64HashMapIterator it = Int64HashMapBegin(&map);
    int64_t key, value;

    EXPECT_FALSE(Int64HashMapIteratorNext(&it, &key, &value))
        << "Iterating over an empty map should immediately return false.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, IteratesOverSingleEntry) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    Int64HashMapSet(&map, 42, 100);

    Int64HashMapIterator it = Int64HashMapBegin(&map);
    int64_t key, value;

    EXPECT_TRUE(Int64HashMapIteratorNext(&it, &key, &value))
        << "The first call to IteratorNext should find the single entry.";

    EXPECT_EQ(key, 42) << "The iterated key should match the inserted key.";

    EXPECT_EQ(value, 100)
        << "The iterated value should match the inserted value.";

    EXPECT_FALSE(Int64HashMapIteratorNext(&it, &key, &value))
        << "The second call to IteratorNext should return false for a "
           "single-entry map.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, IteratesOverMultipleEntries) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    const std::vector<std::pair<int64_t, int64_t>> expected = {
        {10, 100}, {20, 200}, {30, 300}, {40, 400}, {50, 500},
    };

    for (auto &[k, v] : expected) {
        Int64HashMapSet(&map, k, v);
    }

    // Collect all iterated entries.
    std::vector<std::pair<int64_t, int64_t>> actual;
    Int64HashMapIterator it = Int64HashMapBegin(&map);
    int64_t key, value;
    while (Int64HashMapIteratorNext(&it, &key, &value)) {
        actual.emplace_back(key, value);
    }

    // Sort both for comparison since iteration order is unspecified.
    auto sorted_expected = expected;
    std::sort(sorted_expected.begin(), sorted_expected.end());
    std::sort(actual.begin(), actual.end());

    EXPECT_EQ(actual.size(), expected.size())
        << "Iteration should visit exactly all inserted entries.";

    EXPECT_EQ(actual, sorted_expected)
        << "Iteration should return all inserted key-value pairs.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, IterationVisitsEveryEntryExactlyOnce) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    constexpr int64_t num_elements = 100;
    for (int64_t i = 0; i < num_elements; ++i) {
        Int64HashMapSet(&map, i, i * 10);
    }

    std::set<int64_t> visited_keys;
    Int64HashMapIterator it = Int64HashMapBegin(&map);
    int64_t key, value;
    while (Int64HashMapIteratorNext(&it, &key, &value)) {
        EXPECT_EQ(visited_keys.count(key), 0u)
            << "Key " << key << " was visited more than once.";
        visited_keys.insert(key);

        EXPECT_EQ(value, key * 10)
            << "Value for key " << key << " should be " << (key * 10) << ".";
    }

    EXPECT_EQ(static_cast<int64_t>(visited_keys.size()), num_elements)
        << "Iteration should visit exactly " << num_elements << " entries.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, IterationWithNullKeyPointer) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    Int64HashMapSet(&map, 42, 100);

    Int64HashMapIterator it = Int64HashMapBegin(&map);
    int64_t value;

    EXPECT_TRUE(Int64HashMapIteratorNext(&it, nullptr, &value))
        << "IteratorNext should succeed with a NULL key pointer.";

    EXPECT_EQ(value, 100)
        << "The value should still be correctly written when key is NULL.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, IterationWithNullValuePointer) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    Int64HashMapSet(&map, 42, 100);

    Int64HashMapIterator it = Int64HashMapBegin(&map);
    int64_t key;

    EXPECT_TRUE(Int64HashMapIteratorNext(&it, &key, nullptr))
        << "IteratorNext should succeed with a NULL value pointer.";

    EXPECT_EQ(key, 42)
        << "The key should still be correctly written when value is NULL.";

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, IterationWithBothPointersNull) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    Int64HashMapSet(&map, 42, 100);

    Int64HashMapIterator it = Int64HashMapBegin(&map);

    EXPECT_TRUE(Int64HashMapIteratorNext(&it, nullptr, nullptr))
        << "IteratorNext should succeed with both pointers NULL.";

    EXPECT_FALSE(Int64HashMapIteratorNext(&it, nullptr, nullptr))
        << "The second call should return false for a single-entry map.";

    Int64HashMapDestroy(&map);
}

// ======================= Stress Testing and Rehashing =======================

TEST(Int64HashMapTest, TriggersNaturalRehashing) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    constexpr int64_t num_elements = 10000;

    for (int64_t i = 0; i < num_elements; ++i) {
        EXPECT_TRUE(Int64HashMapSet(&map, i, i * 7))
            << "The map should dynamically grow and successfully set element "
            << i << " during natural rehashing.";
    }

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, PreservesDataAfterRehashing) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    constexpr int64_t num_elements = 10000;

    for (int64_t i = 0; i < num_elements; ++i) {
        Int64HashMapSet(&map, i, i * 7);
    }

    // Verify all values are correct after rehashing.
    for (int64_t i = 0; i < num_elements; ++i) {
        Int64HashMapIterator it = Int64HashMapGet(&map, i);
        EXPECT_TRUE(Int64HashMapIteratorIsValid(&it))
            << "The map should still contain key " << i << " after rehashing.";

        EXPECT_EQ(Int64HashMapIteratorValue(&it), i * 7)
            << "The value for key " << i
            << " should be preserved after "
               "rehashing.";
    }

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, ContainsHandlesMissingKeysInDenseMap) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    // Insert even keys only.
    for (int64_t i = 0; i < 2000; i += 2) {
        Int64HashMapSet(&map, i, i);
    }

    // Odd keys should not be found.
    for (int64_t i = 1; i < 2000; i += 2) {
        EXPECT_FALSE(Int64HashMapContains(&map, i))
            << "The map should not contain the unadded odd key " << i << ".";
    }

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, OverwritesDuringRehashing) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    constexpr int64_t num_elements = 10000;

    // First pass: insert all.
    for (int64_t i = 0; i < num_elements; ++i) {
        Int64HashMapSet(&map, i, i);
    }

    // Second pass: overwrite all with different values.
    for (int64_t i = 0; i < num_elements; ++i) {
        EXPECT_TRUE(Int64HashMapSet(&map, i, i + 1000000))
            << "Overwriting key " << i << " should succeed.";
    }

    // Verify all values are the overwritten ones.
    for (int64_t i = 0; i < num_elements; ++i) {
        Int64HashMapIterator it = Int64HashMapGet(&map, i);
        EXPECT_TRUE(Int64HashMapIteratorIsValid(&it))
            << "The map should still contain key " << i << ".";

        EXPECT_EQ(Int64HashMapIteratorValue(&it), i + 1000000)
            << "The value for key " << i << " should reflect the overwrite.";
    }

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, HandlesDenseClusters) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    // Dense block of contiguous integers across the zero boundary.
    for (int64_t i = -2500; i <= 2500; ++i) {
        EXPECT_TRUE(Int64HashMapSet(&map, i, -i))
            << "The map should successfully set contiguous key " << i << ".";
    }

    for (int64_t i = -2500; i <= 2500; ++i) {
        Int64HashMapIterator it = Int64HashMapGet(&map, i);
        EXPECT_TRUE(Int64HashMapIteratorIsValid(&it))
            << "The map should contain key " << i
            << " after dense cluster insertion.";

        EXPECT_EQ(Int64HashMapIteratorValue(&it), -i)
            << "The value for key " << i << " should be " << (-i) << ".";
    }

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, HandlesSparseClusters) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    const std::vector<std::pair<int64_t, int64_t>> sparse_entries = {
        {0, 1},
        {1LL << 20, 2},
        {-(1LL << 30), 3},
        {1LL << 40, 4},
        {-(1LL << 50), 5},
        {std::numeric_limits<int64_t>::max(), 6},
    };

    for (auto &[k, v] : sparse_entries) {
        EXPECT_TRUE(Int64HashMapSet(&map, k, v))
            << "The map should successfully set sparse key " << k << ".";
    }

    for (auto &[k, v] : sparse_entries) {
        Int64HashMapIterator it = Int64HashMapGet(&map, k);
        EXPECT_TRUE(Int64HashMapIteratorIsValid(&it))
            << "The map should find sparse key " << k << ".";

        EXPECT_EQ(Int64HashMapIteratorValue(&it), v)
            << "The value for sparse key " << k << " should be " << v << ".";
    }

    Int64HashMapDestroy(&map);
}

TEST(Int64HashMapTest, IterationAfterRehashing) {
    Int64HashMap map;
    Int64HashMapInit(&map, 0.5);

    constexpr int64_t num_elements = 10000;
    for (int64_t i = 0; i < num_elements; ++i) {
        Int64HashMapSet(&map, i, i * 3);
    }

    // Collect all entries via iteration.
    std::set<int64_t> visited_keys;
    Int64HashMapIterator it = Int64HashMapBegin(&map);
    int64_t key, value;
    while (Int64HashMapIteratorNext(&it, &key, &value)) {
        EXPECT_EQ(visited_keys.count(key), 0u)
            << "Key " << key
            << " was visited more than once during "
               "post-rehash iteration.";
        visited_keys.insert(key);

        EXPECT_EQ(value, key * 3)
            << "Value for key " << key << " should be " << (key * 3)
            << " during post-rehash iteration.";
    }

    EXPECT_EQ(static_cast<int64_t>(visited_keys.size()), num_elements)
        << "Iteration after rehashing should visit all " << num_elements
        << " entries.";

    Int64HashMapDestroy(&map);
}

#include <gtest/gtest.h>
#include <immintrin.h>

#include <climits>

extern "C" {
#include "core/hash/x86_simd_two_piece.h"
#include "core/types/gamesman_status.h"
}

// ================= X86SimdTwoPieceHashContextMemoryRequired =================

// Tests the memory requirement for the smallest valid board size.
TEST(X86SimdTwoPieceHashContextTest, ReturnsValidMemoryForMinimumSlots) {
    size_t memory_required = X86SimdTwoPieceHashContextMemoryRequired(1);
    EXPECT_GT(memory_required, 0)
        << "Memory required for the minimum valid board size (1 slot) must be "
           "strictly greater than 0.";
    EXPECT_LE(memory_required, SIZE_MAX)
        << "Memory required for the minimum valid board size (1 slot) must not "
           "be SIZE_MAX, which is used as an error indicator.";
}

// Tests the memory requirement for the largest supported board size.
TEST(X86SimdTwoPieceHashContextTest,
     ReturnsValidMemoryForMaximumSupportedSlots) {
    size_t memory_required = X86SimdTwoPieceHashContextMemoryRequired(
        kX86SimdTwoPieceHashBoardSizeMax);
    EXPECT_GT(memory_required, 0)
        << "Memory required for the maximum supported board size ("
        << kX86SimdTwoPieceHashBoardSizeMax
        << ") must be strictly greater than 0.";
    EXPECT_LE(memory_required, SIZE_MAX)
        << "Memory required for the maximum supported board size ("
        << kX86SimdTwoPieceHashBoardSizeMax
        << ") must not be SIZE_MAX, which is used as an error indicator.";
}

// Tests the error handling and safeguard against negative slot counts.
TEST(X86SimdTwoPieceHashContextTest, HandlesNegativeSlotCounts) {
    EXPECT_EQ(X86SimdTwoPieceHashContextMemoryRequired(-1), SIZE_MAX)
        << "Memory required for a negative slot count should return SIZE_MAX "
           "to indicate an invalid parameter.";
    EXPECT_EQ(X86SimdTwoPieceHashContextMemoryRequired(-10), SIZE_MAX)
        << "Memory required for -10 slots should return SIZE_MAX.";
}

// Tests the error handling and safeguard against unsupported large slot counts.
TEST(X86SimdTwoPieceHashContextTest, HandlesExceedingMaximumSlotCounts) {
    // Assuming the implementation returns 0 to gracefully indicate an invalid
    // parameter.
    EXPECT_EQ(X86SimdTwoPieceHashContextMemoryRequired(
                  kX86SimdTwoPieceHashBoardSizeMax + 1),
              SIZE_MAX)
        << "Memory required for a slot count exceeding the maximum limit ("
        << kX86SimdTwoPieceHashBoardSizeMax << ") should return SIZE_MAX.";
}

// Tests the mathematical consistency of the memory calculation logic.
TEST(X86SimdTwoPieceHashContextTest,
     MemoryRequirementIsMonotonicallyIncreasing) {
    size_t prev_memory = X86SimdTwoPieceHashContextMemoryRequired(1);

    for (int i = 2; i <= kX86SimdTwoPieceHashBoardSizeMax; ++i) {
        size_t current_memory = X86SimdTwoPieceHashContextMemoryRequired(i);
        EXPECT_GE(current_memory, prev_memory)
            << "Memory requirement should be monotonically increasing. Failed "
               "when transitioning from "
            << (i - 1) << " slots to " << i << " slots.";
        prev_memory = current_memory;
    }
}

// Tests the memory calculation for known, standard game dimensions.
TEST(X86SimdTwoPieceHashContextTest,
     MemoryRequirementMatchesExpectedValuesForStandardBoards) {
    size_t ttt_memory =
        X86SimdTwoPieceHashContextMemoryRequired(9);  // Tic-Tac-Toe
    size_t nmm_memory =
        X86SimdTwoPieceHashContextMemoryRequired(24);  // Nine Men's Morris

    EXPECT_GT(ttt_memory, 0) << "Tic-Tac-Toe (9 slots) memory requirement "
                                "should be correctly calculated and non-zero.";
    EXPECT_GT(nmm_memory, 0)
        << "Nine Men's Morris (24 slots) memory requirement should be "
           "correctly calculated and non-zero.";

    EXPECT_GE(nmm_memory, ttt_memory)
        << "A game with 24 slots (Nine Men's Morris) should require at least "
           "as much memory as a game with 9 slots (Tic-Tac-Toe).";
}

// ====================== X86SimdTwoPieceHashContextInit ======================

// Tests that the context initializes successfully for the smallest valid board
// (1x1).
TEST(X86SimdTwoPieceHashContextTest,
     InitializesSuccessfullyForMinimumBoardSize) {
    X86SimdTwoPieceHashContext context;
    Status status = X86SimdTwoPieceHashContextInit(&context, 1, 1);

    EXPECT_EQ(status, kSuccess)
        << "Initialization should succeed for the minimum valid board size of "
           "1 row and 1 column.";

    // Clean up memory if initialization was successful.
    if (status == kSuccess) {
        X86SimdTwoPieceHashContextDestroy(&context);
    }
}

#ifdef GAMESMAN_ENABLE_STRESS_TESTS
// Tests that the context initializes successfully for the largest supported
// layouts (<= 32 slots, max 8 in any dimension).
// Warning: this test requires 32 GiB memory.
TEST(X86SimdTwoPieceHashContextTest,
     InitializesSuccessfullyForMaximumValidBoardSize) {
    X86SimdTwoPieceHashContext context_wide;
    Status status_wide = X86SimdTwoPieceHashContextInit(&context_wide, 4, 8);

    EXPECT_EQ(status_wide, kSuccess)
        << "Initialization should succeed for a 4x8 board, which hits the "
           "maximum 32 slots without exceeding the 8-column limit.";

    if (status_wide == kSuccess) {
        X86SimdTwoPieceHashContextDestroy(&context_wide);
    }

    X86SimdTwoPieceHashContext context_tall;
    Status status_tall = X86SimdTwoPieceHashContextInit(&context_tall, 8, 4);

    EXPECT_EQ(status_tall, kSuccess)
        << "Initialization should succeed for an 8x4 board, which hits the "
           "maximum 32 slots without exceeding the 8-row limit.";

    if (status_tall == kSuccess) {
        X86SimdTwoPieceHashContextDestroy(&context_tall);
    }
}
#endif  // GAMESMAN_ENABLE_STRESS_TESTS

// Tests that the context initializes successfully for a standard intermediate
// square board (e.g., Tic-Tac-Toe).
TEST(X86SimdTwoPieceHashContextTest,
     InitializesSuccessfullyForStandardSquareBoard) {
    X86SimdTwoPieceHashContext context;
    Status status = X86SimdTwoPieceHashContextInit(&context, 3, 3);

    EXPECT_EQ(status, kSuccess)
        << "Initialization should succeed for a standard 3x3 square board "
           "commonly used in games like Tic-Tac-Toe.";

    if (status == kSuccess) {
        X86SimdTwoPieceHashContextDestroy(&context);
    }
}

// Tests that initialization fails and returns an error when the number of rows
// is zero.
TEST(X86SimdTwoPieceHashContextTest, ReturnsErrorForZeroRows) {
    X86SimdTwoPieceHashContext context;
    Status status = X86SimdTwoPieceHashContextInit(&context, 0, 3);

    EXPECT_EQ(status, kIllegalArgumentError)
        << "Initialization should return kIllegalArgumentError when the number "
           "of rows is 0.";
}

// Tests that initialization fails and returns an error when the number of
// columns is zero.
TEST(X86SimdTwoPieceHashContextTest, ReturnsErrorForZeroCols) {
    X86SimdTwoPieceHashContext context;
    Status status = X86SimdTwoPieceHashContextInit(&context, 3, 0);

    EXPECT_EQ(status, kIllegalArgumentError)
        << "Initialization should return kIllegalArgumentError when the number "
           "of columns is 0.";
}

// Tests that initialization fails and returns an error when the number of rows
// is negative.
TEST(X86SimdTwoPieceHashContextTest, ReturnsErrorForNegativeRows) {
    X86SimdTwoPieceHashContext context;
    Status status = X86SimdTwoPieceHashContextInit(&context, -1, 3);

    EXPECT_EQ(status, kIllegalArgumentError)
        << "Initialization should return kIllegalArgumentError when the number "
           "of rows is negative.";
}

// Tests that initialization fails and returns an error when the number of
// columns is negative.
TEST(X86SimdTwoPieceHashContextTest, ReturnsErrorForNegativeCols) {
    X86SimdTwoPieceHashContext context;
    Status status = X86SimdTwoPieceHashContextInit(&context, 3, -1);

    EXPECT_EQ(status, kIllegalArgumentError)
        << "Initialization should return kIllegalArgumentError when the number "
           "of columns is negative.";
}

// Tests that initialization fails when the number of rows exceeds the maximum
// allowed (8), even if total slots are valid.
TEST(X86SimdTwoPieceHashContextTest, ReturnsErrorForRowsExceedingMaximum) {
    X86SimdTwoPieceHashContext context;
    // 9 rows * 2 cols = 18 total slots, which is <= 32, but rows > 8
    Status status = X86SimdTwoPieceHashContextInit(&context, 9, 2);

    EXPECT_EQ(status, kIllegalArgumentError)
        << "Initialization should return kIllegalArgumentError when rows (9) "
           "exceed the maximum limit of 8.";
}

// Tests that initialization fails when the number of columns exceeds the
// maximum allowed (8), even if total slots are valid.
TEST(X86SimdTwoPieceHashContextTest, ReturnsErrorForColsExceedingMaximum) {
    X86SimdTwoPieceHashContext context;
    // 2 rows * 9 cols = 18 total slots, which is <= 32, but cols > 8
    Status status = X86SimdTwoPieceHashContextInit(&context, 2, 9);

    EXPECT_EQ(status, kIllegalArgumentError)
        << "Initialization should return kIllegalArgumentError when columns "
           "(9) exceed the maximum limit of 8.";
}

// Tests that initialization fails when the total number of board slots exceeds
// the absolute maximum (32).
TEST(X86SimdTwoPieceHashContextTest, ReturnsErrorWhenTotalSlotsExceedLimit) {
    X86SimdTwoPieceHashContext context;

    // 6 rows * 6 cols = 36 total slots (> 32)
    Status status = X86SimdTwoPieceHashContextInit(&context, 6, 6);
    EXPECT_EQ(status, kIllegalArgumentError)
        << "Initialization should return kIllegalArgumentError when total "
           "slots (36) exceed the maximum limit of 32.";

    // 8 rows * 8 cols = 64 total slots (> 32)
    status = X86SimdTwoPieceHashContextInit(&context, 8, 8);
    EXPECT_EQ(status, kIllegalArgumentError)
        << "Initialization should return kIllegalArgumentError when total "
           "slots (64) exceed the maximum limit of 32.";
}

// ================== X86SimdTwoPieceHashContextInitIrregular ==================

// Tests that initialization succeeds for the smallest possible irregular board
// (1 slot).
TEST(X86SimdTwoPieceHashContextTest,
     InitializesIrregularSuccessfullyForSingleBitMask) {
    X86SimdTwoPieceHashContext context;
    // A mask with exactly the 0th bit set.
    uint64_t board_mask = 1ULL;

    Status status =
        X86SimdTwoPieceHashContextInitIrregular(&context, board_mask);

    EXPECT_EQ(status, kSuccess)
        << "Initialization should succeed for an irregular board mask with "
           "exactly 1 bit set.";

    if (status == kSuccess) {
        X86SimdTwoPieceHashContextDestroy(&context);
    }
}

#ifdef GAMESMAN_ENABLE_STRESS_TESTS
// Tests that initialization succeeds when exactly 32 bits are set in the mask.
// Warning: this test requires 32 GiB memory.
TEST(X86SimdTwoPieceHashContextTest,
     InitializesIrregularSuccessfullyForMaximumValidBits) {
    X86SimdTwoPieceHashContext context;
    // A mask with exactly the lowest 32 bits set.
    uint64_t board_mask = 0x00000000FFFFFFFFULL;

    Status status =
        X86SimdTwoPieceHashContextInitIrregular(&context, board_mask);

    EXPECT_EQ(status, kSuccess)
        << "Initialization should succeed for an irregular board mask with "
           "exactly 32 bits set (maximum supported size).";

    if (status == kSuccess) {
        X86SimdTwoPieceHashContextDestroy(&context);
    }
}
#endif  // GAMESMAN_ENABLE_STRESS_TESTS

// Tests that initialization succeeds using the real-world Nine Men's Morris
// mask from the documentation.
TEST(X86SimdTwoPieceHashContextTest,
     InitializesIrregularSuccessfullyForStandardGameMask) {
    X86SimdTwoPieceHashContext context;
    // Nine Men's Morris 24-bit mask copied verbatim from the libary
    // documentation.
    uint64_t board_mask =
        0b00000000'01001001'00101010'00011100'01010101'00011100'00101010'01001001;
    Status status =
        X86SimdTwoPieceHashContextInitIrregular(&context, board_mask);

    EXPECT_EQ(status, kSuccess)
        << "Initialization should succeed for the standard Nine Men's Morris "
           "24-bit irregular board mask.";

    if (status == kSuccess) {
        X86SimdTwoPieceHashContextDestroy(&context);
    }
}

// Tests that initialization succeeds for a valid mask where the set bits are
// scattered across the full 64-bit range.
TEST(X86SimdTwoPieceHashContextTest,
     InitializesIrregularSuccessfullyForScatteredMask) {
    X86SimdTwoPieceHashContext context;
    // A mask with exactly 18 bits set, spanning the lowest (0) to the highest
    // (63) bits.
    uint64_t board_mask = 0x8000000000000001ULL | 0x0000FFFF00000000ULL;

    Status status =
        X86SimdTwoPieceHashContextInitIrregular(&context, board_mask);

    EXPECT_EQ(status, kSuccess)
        << "Initialization should succeed for an irregular board mask with "
           "valid but widely scattered bits.";

    if (status == kSuccess) {
        X86SimdTwoPieceHashContextDestroy(&context);
    }
}

// Tests that initialization fails when the irregular board mask has 33 bits
// set, exceeding the 32-slot limit.
TEST(X86SimdTwoPieceHashContextTest, ReturnsErrorForMaskWith33Bits) {
    X86SimdTwoPieceHashContext context;
    // A mask with exactly 33 bits set (the lowest 33 bits: 32 bits from
    // FFFFFFFF plus 1 bit from 1).
    uint64_t board_mask = 0x00000001FFFFFFFFULL;

    Status status =
        X86SimdTwoPieceHashContextInitIrregular(&context, board_mask);

    EXPECT_EQ(status, kIllegalArgumentError)
        << "Initialization should return kIllegalArgumentError when the mask "
           "has 33 bits set, which exceeds the maximum supported size of 32.";
}

// ===================== X86SimdTwoPieceHashContextDestroy =====================

// Tests that a successfully initialized context can be destroyed without
// causing a crash or memory corruption.
TEST(X86SimdTwoPieceHashContextTest, SafelyDestroysInitializedContext) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully for the destroy test "
           "to be valid.";

    // If it crashes or faults here, the test fails automatically.
    // We wrap it in an EXPECT_NO_FATAL_FAILURE to explicitly document our
    // assertion.
    EXPECT_NO_FATAL_FAILURE(X86SimdTwoPieceHashContextDestroy(&context))
        << "Destroying a valid, initialized context should complete safely.";
}

// Tests that the destroy function handles a NULL pointer gracefully without
// crashing.
TEST(X86SimdTwoPieceHashContextTest, DoesNotCrashOnNullPointer) {
    EXPECT_NO_FATAL_FAILURE(X86SimdTwoPieceHashContextDestroy(nullptr))
        << "Passing a null pointer to the destroy function should immediately "
           "return and not cause a segmentation fault.";
}

// Tests that calling the destroy function multiple times on the same context is
// safe.
TEST(X86SimdTwoPieceHashContextTest, DoesNotCrashOnMultipleDestroys) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // First destruction
    X86SimdTwoPieceHashContextDestroy(&context);

    // Second destruction of the same context
    EXPECT_NO_FATAL_FAILURE(X86SimdTwoPieceHashContextDestroy(&context))
        << "Calling destroy on an already deallocated context should be a safe "
           "no-op and must not result in a double-free crash.";
}

// ============== X86SimdTwoPieceHashGetNumPositions + FixedTurn ==============

// Verifies that the fixed-turn function correctly computes the exact
// mathematical permutations for placing two types of pieces on the board.
TEST(X86SimdTwoPieceHashTest, CombinatorialAccuracyFixedTurn) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // 0 X's, 0 O's -> C(9,0) * C(9,0) = 1
    EXPECT_EQ(X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, 0, 0), 1)
        << "Empty board should have exactly 1 permutation.";

    // 1 X, 0 O's -> C(9,1) * C(8,0) = 9
    EXPECT_EQ(X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, 1, 0), 9)
        << "1 piece on a 9-slot board should have 9 permutations.";

    // 2 X's, 1 O -> C(9,2) * C(7,1) = 36 * 7 = 252
    EXPECT_EQ(X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, 2, 1), 252)
        << "2 X's and 1 O did not match combinatorial expectation (252).";

    // 2 X's, 2 O's -> C(9,2) * C(7,2) = 36 * 21 = 756
    EXPECT_EQ(X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, 2, 2), 756)
        << "2 X's and 2 O's did not match combinatorial expectation (756).";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Ensures that the variable-turn function accurately reflects the
// doubled state space caused by appending the turn bit.
TEST(X86SimdTwoPieceHashTest, VariableTurnMultiplierRelationship) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // Test combinations of up to 4 pieces of each type
    for (int num_x = 0; num_x <= 4; ++num_x) {
        for (int num_o = 0; num_o <= 4; ++num_o) {
            if (num_x + num_o > 9) continue;

            int64_t fixed_positions =
                X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, num_x,
                                                            num_o);
            int64_t turned_positions =
                X86SimdTwoPieceHashGetNumPositions(&context, num_x, num_o);

            EXPECT_EQ(turned_positions, fixed_positions * 2)
                << "Variable turn positions must be exactly double fixed turn "
                   "positions for X="
                << num_x << ", O=" << num_o;
        }
    }

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Verifies that the capacity calculation respects irregular board masks
// and only counts playable slots, not the raw bounding box size.
TEST(X86SimdTwoPieceHashTest, IrregularBoardCapacity) {
    X86SimdTwoPieceHashContext context;
    // 4x4 board (16 possible slots), but masked to only have 10 valid playable
    // slots. Mask 0x03FF has exactly 10 bits set to 1.
    uint32_t mask = 0x03FF;
    ASSERT_EQ(X86SimdTwoPieceHashContextInitIrregular(&context, mask), kSuccess)
        << "Setup: Irregular context must initialize successfully.";

    // 2 X's, 2 O's on 10 valid slots -> C(10,2) * C(8,2) = 45 * 28 = 1260
    EXPECT_EQ(X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, 2, 2), 1260)
        << "Irregular board failed to calculate permutations based strictly on "
           "the mask bit count.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

#ifdef GAMESMAN_ENABLE_STRESS_TESTS
// Tests the upper limit of the hashing system (32 slots) to ensure there is
// no integer overflow when calculating heavily populated boards.
// Warning: this test requires 32 GiB memory.
TEST(X86SimdTwoPieceHashTest, MaximumCapacityBoundary) {
    X86SimdTwoPieceHashContext context;
    // Initialize a full 32-slot board context (e.g., 8x4)
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 8, 4), kSuccess)
        << "Setup: Context must initialize successfully.";

    // Densest possible configuration: 16 X's and 16 O's
    int num_x = 16;
    int num_o = 16;

    // C(32, 16) * C(16, 16) = 601,080,390 * 1 = 601,080,390
    int64_t expected_fixed = 601080390LL;
    int64_t expected_turned = 1202160780LL;

    EXPECT_EQ(
        X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, num_x, num_o),
        expected_fixed)
        << "Overflow or logic error on fixed-turn 32-slot max capacity.";

    EXPECT_EQ(X86SimdTwoPieceHashGetNumPositions(&context, num_x, num_o),
              expected_turned)
        << "Overflow or logic error on variable-turn 32-slot max capacity.";

    X86SimdTwoPieceHashContextDestroy(&context);
}
#endif  // GAMESMAN_ENABLE_STRESS_TESTS

// ==================== X86SimdTwoPieceHashHashFixedTurnMem ====================

// Tests that a board with zero pieces of either type correctly hashes to 0.
TEST(X86SimdTwoPieceHashTest, HashesEmptyBoardToZero) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    uint64_t patterns[2] = {0, 0};
    Position hash = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);

    EXPECT_EQ(hash, 0)
        << "An empty board (0 X's and 0 O's) should always hash to exactly 0.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that the lowest possible bit configuration for a tier hashes to 0.
TEST(X86SimdTwoPieceHashTest, HashesFirstLexicographicalPositionToZero) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // For a single piece (1 X, 0 O), the lowest lexicographical position is
    // occupying the 0th bit.
    uint64_t patterns[2] = {1ULL, 0};
    Position hash = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);

    EXPECT_EQ(hash, 0) << "The first lexicographical configuration (pieces in "
                          "the lowest available bits) should hash to 0.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that the highest possible bit configuration for a tier hashes to the
// total number of positions minus 1.
TEST(X86SimdTwoPieceHashTest, HashesLastLexicographicalPositionToMaxMinusOne) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // Total positions for 1 X and 0 O on a 9-slot board is 9.
    int64_t max_positions =
        X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, 1, 0);
    ASSERT_EQ(max_positions, 9)
        << "Setup: A 3x3 board should have exactly 9 positions for 1 piece.";

    // For a single piece (1 X, 0 O), the highest lexicographical position is
    // occupying the 18th bit.
    uint64_t patterns[2] = {1ULL << 18, 0};
    Position hash = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);

    EXPECT_EQ(hash, max_positions - 1)
        << "The last lexicographical configuration (pieces in the highest "
           "available bits) should hash to max_positions - 1.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that the hash function is deterministic and yields the same result for
// the same input.
TEST(X86SimdTwoPieceHashTest, HashIsDeterministic) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // Arbitrary pattern: X occupies slots 0 and 2; O occupies slot 1.
    uint64_t patterns[2] = {0b101ULL, 0b010ULL};

    Position hash1 = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);
    Position hash2 = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);
    Position hash3 = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);

    EXPECT_EQ(hash1, hash2)
        << "Hash function must be deterministic; hashing the same pattern "
           "multiple times should yield identical results.";
    EXPECT_EQ(hash2, hash3) << "Hash function must be deterministic across "
                               "three consecutive calls.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that a board with zero pieces correctly hashes to 0 on an irregular
// board.
TEST(X86SimdTwoPieceHashTest, HashesEmptyIrregularBoardToZero) {
    X86SimdTwoPieceHashContext context;
    // Irregular mask with 4 active slots: bits 1, 3, 5, and 7.
    uint64_t board_mask = 0xAAULL;
    ASSERT_EQ(X86SimdTwoPieceHashContextInitIrregular(&context, board_mask),
              kSuccess)
        << "Setup: Context must initialize successfully.";

    uint64_t patterns[2] = {0, 0};
    Position hash = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);

    EXPECT_EQ(hash, 0) << "An empty irregular board (0 X's and 0 O's) should "
                          "always hash to exactly 0.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that the lowest possible valid bit configuration for a tier hashes to 0
// on an irregular board.
TEST(X86SimdTwoPieceHashTest,
     HashesFirstLexicographicalPositionOnIrregularBoardToZero) {
    X86SimdTwoPieceHashContext context;
    // Irregular mask with 4 active slots: bits 1, 3, 5, and 7.
    uint64_t board_mask = 0xAAULL;
    ASSERT_EQ(X86SimdTwoPieceHashContextInitIrregular(&context, board_mask),
              kSuccess)
        << "Setup: Context must initialize successfully.";

    // For a single piece (1 X, 0 O), the lowest lexicographical position is
    // occupying the lowest available set bit (bit 1).
    uint64_t patterns[2] = {1ULL << 1, 0};
    Position hash = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);

    EXPECT_EQ(hash, 0)
        << "The first lexicographical configuration on an irregular board "
           "(pieces in the lowest available masked bits) should hash to 0.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that the highest possible valid bit configuration for a tier hashes to
// max minus one on an irregular board.
TEST(X86SimdTwoPieceHashTest,
     HashesLastLexicographicalPositionOnIrregularBoardToMaxMinusOne) {
    X86SimdTwoPieceHashContext context;
    // Irregular mask with 4 active slots: bits 1, 3, 5, and 7.
    uint64_t board_mask = 0xAAULL;
    ASSERT_EQ(X86SimdTwoPieceHashContextInitIrregular(&context, board_mask),
              kSuccess)
        << "Setup: Context must initialize successfully.";

    // Total positions for 1 X and 0 O on a 4-slot board is 4.
    int64_t max_positions =
        X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, 1, 0);
    ASSERT_EQ(max_positions, 4) << "Setup: A 4-slot irregular board should "
                                   "have exactly 4 positions for 1 piece.";

    // For a single piece (1 X, 0 O), the highest lexicographical position is
    // occupying the highest available set bit (bit 7).
    uint64_t patterns[2] = {1ULL << 7, 0};
    Position hash = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);

    EXPECT_EQ(hash, max_positions - 1)
        << "The last lexicographical configuration on an irregular board "
           "(pieces in the highest available masked bits) should hash to "
           "max_positions - 1.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that the hash function is deterministic on an irregular board.
TEST(X86SimdTwoPieceHashTest, HashIsDeterministicOnIrregularBoard) {
    X86SimdTwoPieceHashContext context;
    // Irregular mask with 4 active slots: bits 1, 3, 5, and 7.
    uint64_t board_mask = 0xAAULL;
    ASSERT_EQ(X86SimdTwoPieceHashContextInitIrregular(&context, board_mask),
              kSuccess)
        << "Setup: Context must initialize successfully.";

    // Arbitrary pattern: X occupies bit 1; O occupies bit 5.
    uint64_t patterns[2] = {1ULL << 1, 1ULL << 5};

    Position hash1 = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);
    Position hash2 = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);
    Position hash3 = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);

    EXPECT_EQ(hash1, hash2)
        << "Hash function must be deterministic on irregular boards; hashing "
           "the same pattern multiple times should yield identical results.";
    EXPECT_EQ(hash2, hash3) << "Hash function must be deterministic across "
                               "three consecutive calls.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// =================== X86SimdTwoPieceHashUnhashFixedTurnMem ===================

// Tests that unhashing a hash value of 0 yields the first lexicographical
// configuration.
TEST(X86SimdTwoPieceHashTest, UnhashesZeroToFirstLexicographicalPosition) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // Initialize with garbage to ensure the function properly overwrites the
    // data.
    uint64_t patterns[2] = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};

    // Unhash position with 1 X and 0 O from hash 0
    X86SimdTwoPieceHashUnhashFixedTurnMem(&context, 0, 1, 0, patterns);

    EXPECT_EQ(patterns[0], 1ULL) << "Unhashing 0 for 1 piece should place it "
                                    "in the lowest available bit (bit 0).";
    EXPECT_EQ(patterns[1], 0ULL)
        << "The second piece pattern should be completely empty.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that unhashing the maximum hash value yields the last lexicographical
// configuration.
TEST(X86SimdTwoPieceHashTest,
     UnhashesMaxMinusOneToLastLexicographicalPosition) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // Total positions for 1 X and 0 O on a 9-slot board is 9.
    int64_t max_positions =
        X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, 1, 0);
    ASSERT_EQ(max_positions, 9)
        << "Setup: A 3x3 board should have exactly 9 positions for 1 piece.";

    uint64_t patterns[2] = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};

    // Unhash position with 1 X and 0 O from the maximum valid hash
    X86SimdTwoPieceHashUnhashFixedTurnMem(&context, max_positions - 1, 1, 0,
                                          patterns);
    EXPECT_EQ(patterns[0], 1ULL << 18)
        << "Unhashing max_positions - 1 for 1 piece on a 3x3 board should "
           "place it in the highest available bit (bit 18 due to 8x8 padding).";
    EXPECT_EQ(patterns[1], 0ULL)
        << "The second piece pattern should be completely empty.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that unhashing a tier with zero pieces yields completely empty
// patterns.
TEST(X86SimdTwoPieceHashTest, UnhashesEmptyBoardCorrectly) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    uint64_t patterns[2] = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};

    // Unhash position with 0 X and 0 O from hash 0
    X86SimdTwoPieceHashUnhashFixedTurnMem(&context, 0, 0, 0, patterns);

    EXPECT_EQ(patterns[0], 0ULL)
        << "Unhashing an empty board should yield 0 for the first pattern.";
    EXPECT_EQ(patterns[1], 0ULL)
        << "Unhashing an empty board should yield 0 for the second pattern.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that unhashing a tier with zero pieces on an irregular board yields
// completely empty patterns.
TEST(X86SimdTwoPieceHashTest, UnhashesEmptyIrregularBoardCorrectly) {
    X86SimdTwoPieceHashContext context;
    // Irregular mask with 4 active slots: bits 1, 3, 5, and 7.
    uint64_t board_mask = 0xAAULL;
    ASSERT_EQ(X86SimdTwoPieceHashContextInitIrregular(&context, board_mask),
              kSuccess)
        << "Setup: Context must initialize successfully.";

    uint64_t patterns[2] = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};

    // Unhash position with 0 X and 0 O from hash 0
    X86SimdTwoPieceHashUnhashFixedTurnMem(&context, 0, 0, 0, patterns);

    EXPECT_EQ(patterns[0], 0ULL) << "Unhashing an empty irregular board should "
                                    "yield 0 for the first pattern.";
    EXPECT_EQ(patterns[1], 0ULL) << "Unhashing an empty irregular board should "
                                    "yield 0 for the second pattern.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that unhashing a hash value of 0 on an irregular board yields the first
// lexicographical configuration.
TEST(X86SimdTwoPieceHashTest,
     UnhashesZeroToFirstLexicographicalPositionOnIrregularBoard) {
    X86SimdTwoPieceHashContext context;
    // Irregular mask with 4 active slots: bits 1, 3, 5, and 7.
    uint64_t board_mask = 0xAAULL;
    ASSERT_EQ(X86SimdTwoPieceHashContextInitIrregular(&context, board_mask),
              kSuccess)
        << "Setup: Context must initialize successfully.";

    uint64_t patterns[2] = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};

    // Unhash position with 1 X and 0 O from hash 0
    X86SimdTwoPieceHashUnhashFixedTurnMem(&context, 0, 1, 0, patterns);

    EXPECT_EQ(patterns[0], 1ULL << 1)
        << "Unhashing 0 for 1 piece on an irregular board should place it in "
           "the lowest available masked bit (bit 1).";
    EXPECT_EQ(patterns[1], 0ULL)
        << "The second piece pattern should be completely empty.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that unhashing the maximum hash value on an irregular board yields the
// last lexicographical configuration.
TEST(X86SimdTwoPieceHashTest,
     UnhashesMaxMinusOneToLastLexicographicalPositionOnIrregularBoard) {
    X86SimdTwoPieceHashContext context;
    // Irregular mask with 4 active slots: bits 1, 3, 5, and 7.
    uint64_t board_mask = 0xAAULL;
    ASSERT_EQ(X86SimdTwoPieceHashContextInitIrregular(&context, board_mask),
              kSuccess)
        << "Setup: Context must initialize successfully.";

    // Total positions for 1 X and 0 O on a 4-slot board is 4.
    int64_t max_positions =
        X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, 1, 0);
    ASSERT_EQ(max_positions, 4) << "Setup: A 4-slot irregular board should "
                                   "have exactly 4 positions for 1 piece.";

    uint64_t patterns[2] = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};

    // Unhash position with 1 X and 0 O from the maximum valid hash
    X86SimdTwoPieceHashUnhashFixedTurnMem(&context, max_positions - 1, 1, 0,
                                          patterns);

    EXPECT_EQ(patterns[0], 1ULL << 7)
        << "Unhashing max_positions - 1 for 1 piece on an irregular board "
           "should place it in the highest available masked bit (bit 7).";
    EXPECT_EQ(patterns[1], 0ULL)
        << "The second piece pattern should be completely empty.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// =================== Unhash + HashFixedTurnMem Round-Trips ===================

// Tests that a single piece placed at various valid bit positions hashes and
// unhashes back to the exact same pattern.
TEST(X86SimdTwoPieceHashTest, RoundTripsSinglePieceBoard) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // Test a single X piece (num_x = 1, num_o = 0)
    int num_x = 1;
    int num_o = 0;

    // Iterate through all 9 valid slots on a 3x3 board (padded to 8x8)
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            int bit_index = (r * 8) + c;
            uint64_t original_patterns[2] = {1ULL << bit_index, 0};

            // 1. Hash the original pattern
            Position hash = X86SimdTwoPieceHashHashFixedTurnMem(
                &context, original_patterns);

            // 2. Unhash back to patterns
            uint64_t unhashed_patterns[2] = {0xFFFFFFFFFFFFFFFFULL,
                                             0xFFFFFFFFFFFFFFFFULL};
            X86SimdTwoPieceHashUnhashFixedTurnMem(&context, hash, num_x, num_o,
                                                  unhashed_patterns);

            // 3. Verify pattern integrity
            EXPECT_EQ(unhashed_patterns[0], original_patterns[0])
                << "Pattern 0 mismatch at row " << r << ", col " << c
                << " (bit " << bit_index << ").";
            EXPECT_EQ(unhashed_patterns[1], original_patterns[1])
                << "Pattern 1 mismatch at row " << r << ", col " << c
                << " (bit " << bit_index << ").";
        }
    }

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests a scenario where num_x + num_o equals the total board size (no empty
// slots left).
TEST(X86SimdTwoPieceHashTest, RoundTripsFullBoard) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // Total slots = 9. Fill the board with 5 X's and 4 O's.
    int num_x = 5;
    int num_o = 4;

    // Create a checkered pattern on the 3x3 board (accounting for 8x8 padding)
    // Row 0: X O X -> bits 0, 1, 2
    // Row 1: O X O -> bits 8, 9, 10
    // Row 2: X O X -> bits 16, 17, 18
    uint64_t x_pattern =
        (1ULL << 0) | (1ULL << 2) | (1ULL << 9) | (1ULL << 16) | (1ULL << 18);
    uint64_t o_pattern =
        (1ULL << 1) | (1ULL << 8) | (1ULL << 10) | (1ULL << 17);
    uint64_t original_patterns[2] = {x_pattern, o_pattern};

    // 1. Hash the full board
    Position hash =
        X86SimdTwoPieceHashHashFixedTurnMem(&context, original_patterns);

    // 2. Unhash back to patterns
    uint64_t unhashed_patterns[2] = {0, 0};
    X86SimdTwoPieceHashUnhashFixedTurnMem(&context, hash, num_x, num_o,
                                          unhashed_patterns);

    // 3. Verify perfect reconstruction
    EXPECT_EQ(unhashed_patterns[0], original_patterns[0])
        << "Full board X pattern failed to round trip correctly.";
    EXPECT_EQ(unhashed_patterns[1], original_patterns[1])
        << "Full board O pattern failed to round trip correctly.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

#ifdef GAMESMAN_ENABLE_STRESS_TESTS
// Uses a 32-slot board initialized context to verify integrity at the maximum
// supported size limit.
// Warning: this test requires 32 GiB memory.
TEST(X86SimdTwoPieceHashTest, RoundTripsMaxSupportedBoard) {
    X86SimdTwoPieceHashContext context;
    // 4 rows x 8 cols = 32 slots, which is the maximum board size for a 64-bit
    // bitboard because padding doesn't push the highest bit past bit 63. For a
    // 4x8 board, the bits used are exactly 0 through 31.
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 4, 8), kSuccess)
        << "Setup: Context must initialize successfully at maximum size.";

    int num_x = 3;
    int num_o = 3;

    // Place pieces at boundaries and middle points.
    // X at bits: 0 (r0c0), 15 (r1c7), 31 (r3c7 - max bit)
    // O at bits: 1 (r0c1), 16 (r2c0), 30 (r3c6)
    uint64_t original_patterns[2] = {(1ULL << 0) | (1ULL << 15) | (1ULL << 31),
                                     (1ULL << 1) | (1ULL << 16) | (1ULL << 30)};

    // 1. Hash the max board pattern
    Position hash =
        X86SimdTwoPieceHashHashFixedTurnMem(&context, original_patterns);

    // 2. Unhash back to patterns
    uint64_t unhashed_patterns[2] = {0, 0};
    X86SimdTwoPieceHashUnhashFixedTurnMem(&context, hash, num_x, num_o,
                                          unhashed_patterns);

    // 3. Verify pattern integrity at the 32-slot limit
    EXPECT_EQ(unhashed_patterns[0], original_patterns[0])
        << "Max board X pattern failed to round trip correctly.";
    EXPECT_EQ(unhashed_patterns[1], original_patterns[1])
        << "Max board O pattern failed to round trip correctly.";

    X86SimdTwoPieceHashContextDestroy(&context);
}
#endif  // GAMESMAN_ENABLE_STRESS_TESTS

// Tests that Unhash followed by Hash returns the original hash value for all
// valid positions on a regular board.
TEST(X86SimdTwoPieceHashTest, RoundTripBijectionRegularBoard) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // Test with 2 X's and 1 O.
    // Total valid positions: C(9, 2) * C(7, 1) = 36 * 7 = 252.
    int num_x = 2;
    int num_o = 1;
    int64_t max_positions =
        X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, num_x, num_o);
    ASSERT_EQ(max_positions, 252) << "Setup: Expected exactly 252 combinations "
                                     "for 2 X's and 1 O on a 3x3 board.";

    for (Position original_hash = 0; original_hash < max_positions;
         ++original_hash) {
        uint64_t patterns[2] = {0, 0};

        // 1. Unhash to get the bitboards
        X86SimdTwoPieceHashUnhashFixedTurnMem(&context, original_hash, num_x,
                                              num_o, patterns);

        // 2. Hash the resulting bitboards back to a position index
        Position rehashed =
            X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);

        // 3. Verify the round trip matches
        ASSERT_EQ(rehashed, original_hash)
            << "Round trip failed! Hash -> Unhash -> Hash did not produce the "
               "original hash. "
            << "Original Hash: " << original_hash << ", Rehashed: " << rehashed
            << ", Pattern 0: 0x" << std::hex << patterns[0] << ", Pattern 1: 0x"
            << patterns[1] << std::dec;
    }

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Tests that Unhash followed by Hash returns the original hash value for all
// valid positions on an irregular board.
TEST(X86SimdTwoPieceHashTest, RoundTripBijectionIrregularBoard) {
    X86SimdTwoPieceHashContext context;
    // Irregular mask with 4 active slots: bits 1, 3, 5, and 7.
    uint64_t board_mask = 0xAAULL;
    ASSERT_EQ(X86SimdTwoPieceHashContextInitIrregular(&context, board_mask),
              kSuccess)
        << "Setup: Context must initialize successfully.";

    // Test with 2 X's and 1 O.
    // Total valid positions: C(4, 2) * C(2, 1) = 6 * 2 = 12.
    int num_x = 2;
    int num_o = 1;
    int64_t max_positions =
        X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, num_x, num_o);
    ASSERT_EQ(max_positions, 12) << "Setup: Expected exactly 12 combinations "
                                    "for 2 X's and 1 O on a 4-slot board.";

    for (Position original_hash = 0; original_hash < max_positions;
         ++original_hash) {
        uint64_t patterns[2] = {0, 0};

        // 1. Unhash to get the bitboards
        X86SimdTwoPieceHashUnhashFixedTurnMem(&context, original_hash, num_x,
                                              num_o, patterns);

        // 2. Hash the resulting bitboards back to a position index
        Position rehashed =
            X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);

        // 3. Verify the round trip matches
        ASSERT_EQ(rehashed, original_hash)
            << "Round trip failed on irregular board! Hash -> Unhash -> Hash "
               "did not produce the original hash. "
            << "Original Hash: " << original_hash << ", Rehashed: " << rehashed
            << ", Pattern 0: 0x" << std::hex << patterns[0] << ", Pattern 1: 0x"
            << patterns[1] << std::dec;
    }

    X86SimdTwoPieceHashContextDestroy(&context);
}

// = X86SimdTwoPieceHashHashFixedTurnMem + X86SimdTwoPieceHashUnhashFixedTurn =

// Verifies that passing data via a SIMD register yields the exact same hash as
// passing the same data via a memory array.
TEST(X86SimdTwoPieceHashTest, HashEquivalenceToMem) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // Arbitrary pattern for 3x3 padded layout
    uint64_t x_pattern = (1ULL << 0) | (1ULL << 8) | (1ULL << 16);
    uint64_t o_pattern = (1ULL << 1) | (1ULL << 9);
    uint64_t patterns[2] = {x_pattern, o_pattern};

    // _mm_set_epi64x takes (high, low).
    // We want lane 0 (low) to be X and lane 1 (high) to be O.
    __m128i simd_patterns = _mm_set_epi64x(o_pattern, x_pattern);

    Position mem_hash = X86SimdTwoPieceHashHashFixedTurnMem(&context, patterns);
    Position simd_hash =
        X86SimdTwoPieceHashHashFixedTurn(&context, simd_patterns);

    EXPECT_EQ(simd_hash, mem_hash)
        << "SIMD hash should exactly match the memory array hash.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Verifies that unhashing to a SIMD register populates the 64-bit lanes exactly
// as the memory function populates the array.
TEST(X86SimdTwoPieceHashTest, UnhashEquivalenceToMem) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    int num_x = 2;
    int num_o = 2;
    Position test_hash = 42;  // Arbitrary valid hash index

    uint64_t patterns[2] = {0, 0};
    X86SimdTwoPieceHashUnhashFixedTurnMem(&context, test_hash, num_x, num_o,
                                          patterns);

    __m128i simd_patterns =
        X86SimdTwoPieceHashUnhashFixedTurn(&context, test_hash, num_x, num_o);

    // Extract lanes to compare with memory output
    uint64_t simd_x = _mm_extract_epi64(simd_patterns, 0);
    uint64_t simd_o = _mm_extract_epi64(simd_patterns, 1);

    EXPECT_EQ(simd_x, patterns[0])
        << "Lane 0 (X pattern) mismatch between SIMD and Mem unhash.";
    EXPECT_EQ(simd_o, patterns[1])
        << "Lane 1 (O pattern) mismatch between SIMD and Mem unhash.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// End-to-end verification of the SIMD pipeline on a standard board.
TEST(X86SimdTwoPieceHashTest, RoundTripSimdRegistersRegularBoard) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    int num_x = 2;
    int num_o = 1;
    int64_t max_positions =
        X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, num_x, num_o);

    for (Position original_hash = 0; original_hash < max_positions;
         ++original_hash) {
        // 1. Unhash directly to a SIMD register
        __m128i simd_patterns = X86SimdTwoPieceHashUnhashFixedTurn(
            &context, original_hash, num_x, num_o);

        // 2. Hash the register back immediately
        Position rehashed =
            X86SimdTwoPieceHashHashFixedTurn(&context, simd_patterns);

        // 3. Verify
        ASSERT_EQ(rehashed, original_hash)
            << "SIMD Round trip failed! Original Hash: " << original_hash
            << ", Rehashed: " << rehashed;
    }

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Ensures the SIMD register pipeline correctly handles masked bits on an
// irregular board.
TEST(X86SimdTwoPieceHashTest, RoundTripSimdRegistersIrregularBoard) {
    X86SimdTwoPieceHashContext context;
    uint64_t board_mask = 0xAAULL;  // Irregular mask
    ASSERT_EQ(X86SimdTwoPieceHashContextInitIrregular(&context, board_mask),
              kSuccess)
        << "Setup: Context must initialize successfully.";

    int num_x = 2;
    int num_o = 1;
    int64_t max_positions =
        X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, num_x, num_o);

    for (Position original_hash = 0; original_hash < max_positions;
         ++original_hash) {
        __m128i simd_patterns = X86SimdTwoPieceHashUnhashFixedTurn(
            &context, original_hash, num_x, num_o);
        Position rehashed =
            X86SimdTwoPieceHashHashFixedTurn(&context, simd_patterns);

        ASSERT_EQ(rehashed, original_hash)
            << "SIMD Irregular Round trip failed! Original Hash: "
            << original_hash << ", Rehashed: " << rehashed;
    }

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Verifies the zero-state behavior using SIMD intrinsics.
TEST(X86SimdTwoPieceHashTest, EmptyBoardSimdRegisters) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // 1. Hash zero test
    __m128i empty_simd = _mm_setzero_si128();
    Position hash = X86SimdTwoPieceHashHashFixedTurn(&context, empty_simd);
    EXPECT_EQ(hash, 0)
        << "Hashing a zeroed SIMD register should return hash 0.";

    // 2. Unhash zero test
    __m128i unhashed_simd =
        X86SimdTwoPieceHashUnhashFixedTurn(&context, 0, 0, 0);
    EXPECT_EQ(_mm_extract_epi64(unhashed_simd, 0), 0ULL)
        << "Unhashing an empty board should yield 0 in lane 0.";
    EXPECT_EQ(_mm_extract_epi64(unhashed_simd, 1), 0ULL)
        << "Unhashing an empty board should yield 0 in lane 1.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

#ifdef GAMESMAN_ENABLE_STRESS_TESTS
// Tests the upper limit (32 slots) ensuring the highest bit in both 64-bit
// lanes of the __m128i register are processed correctly without bleeding into
// one another.
TEST(X86SimdTwoPieceHashTest, MaxSupportedBoardSimdRegisters) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 4, 8), kSuccess)
        << "Setup: Context must initialize successfully at maximum size.";

    // Use the absolute boundaries of the 32-slot limit (bit 31)
    uint64_t x_pattern = (1ULL << 31) | (1ULL << 0);
    uint64_t o_pattern = (1ULL << 30) | (1ULL << 15);

    // _mm_set_epi64x takes (lane 1, lane 0) -> (O, X)
    __m128i original_simd = _mm_set_epi64x(o_pattern, x_pattern);

    // Hash and Unhash using the SIMD functions
    Position hash = X86SimdTwoPieceHashHashFixedTurn(&context, original_simd);
    __m128i unhashed_simd =
        X86SimdTwoPieceHashUnhashFixedTurn(&context, hash, 2, 2);

    // Verify boundary bits were preserved perfectly
    EXPECT_EQ(_mm_extract_epi64(unhashed_simd, 0), x_pattern)
        << "Lower lane (X pattern) boundary bits failed to survive SIMD round "
           "trip.";
    EXPECT_EQ(_mm_extract_epi64(unhashed_simd, 1), o_pattern)
        << "Upper lane (O pattern) boundary bits failed to survive SIMD round "
           "trip.";

    X86SimdTwoPieceHashContextDestroy(&context);
}
#endif  // GAMESMAN_ENABLE_STRESS_TESTS

// ========= X86SimdTwoPieceHashHashMem + X86SimdTwoPieceHashUnhashMem =========

// Verifies that the turn bit is correctly integrated into the hash
// and can be perfectly extracted without corrupting the board patterns.
TEST(X86SimdTwoPieceHashTest, TurnBitEncodingAndRecovery) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // Arbitrary valid pattern for a 3x3 layout (e.g., 2 X's, 1 O)
    uint64_t x_pattern = (1ULL << 0) | (1ULL << 8);
    uint64_t o_pattern = (1ULL << 1);
    uint64_t original_patterns[2] = {x_pattern, o_pattern};

    // Hash the same patterns with both turn 0 and turn 1
    Position hash_turn_0 =
        X86SimdTwoPieceHashHashMem(&context, original_patterns, 0);
    Position hash_turn_1 =
        X86SimdTwoPieceHashHashMem(&context, original_patterns, 1);

    EXPECT_NE(hash_turn_0, hash_turn_1)
        << "Hashes for the exact same board state but different turns must be "
           "distinct.";

    // Unhash turn 0
    uint64_t unhashed_patterns_0[2] = {0, 0};
    int recovered_turn_0 = X86SimdTwoPieceHashGetTurn(hash_turn_0);
    EXPECT_EQ(recovered_turn_0, 0) << "Failed to recover turn 0.";

    X86SimdTwoPieceHashUnhashMem(&context, hash_turn_0, 2, 1,
                                 unhashed_patterns_0);
    EXPECT_EQ(unhashed_patterns_0[0], x_pattern)
        << "X pattern corrupted on turn 0 unhash.";
    EXPECT_EQ(unhashed_patterns_0[1], o_pattern)
        << "O pattern corrupted on turn 0 unhash.";

    // Unhash turn 1
    uint64_t unhashed_patterns_1[2] = {0, 0};
    int recovered_turn_1 = X86SimdTwoPieceHashGetTurn(hash_turn_1);
    EXPECT_EQ(recovered_turn_1, 1) << "Failed to recover turn 1.";

    X86SimdTwoPieceHashUnhashMem(&context, hash_turn_1, 2, 1,
                                 unhashed_patterns_1);
    EXPECT_EQ(unhashed_patterns_1[0], x_pattern)
        << "X pattern corrupted on turn 1 unhash.";
    EXPECT_EQ(unhashed_patterns_1[1], o_pattern)
        << "O pattern corrupted on turn 1 unhash.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Ensures the round-trip bijection holds across the entire doubled state space
// for a specific piece count.
TEST(X86SimdTwoPieceHashTest, ExhaustiveTurnRoundTripBijection) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    int num_x = 2;
    int num_o = 2;

    // The variable-turn state space is exactly double the fixed-turn state
    // space
    int64_t max_fixed_positions =
        X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, num_x, num_o);
    int64_t max_turned_positions = max_fixed_positions * 2;

    for (Position original_hash = 0; original_hash < max_turned_positions;
         ++original_hash) {
        uint64_t patterns[2] = {0, 0};

        // 1. Unhash the current index, recovering both the board patterns and
        // the turn
        int turn = X86SimdTwoPieceHashGetTurn(original_hash);
        X86SimdTwoPieceHashUnhashMem(&context, original_hash, num_x, num_o,
                                     patterns);

        // 2. Validate turn bit is structurally sound
        ASSERT_TRUE(turn == 0 || turn == 1)
            << "Recovered turn must be strictly 0 or 1. Got: " << turn
            << " at hash " << original_hash;

        // 3. Re-hash the recovered state
        Position rehashed =
            X86SimdTwoPieceHashHashMem(&context, patterns, turn);

        // 4. Verify absolute bijection
        ASSERT_EQ(rehashed, original_hash)
            << "Variable-turn round trip failed! Original Hash: "
            << original_hash << ", Rehashed: " << rehashed
            << ", Recovered Turn: " << turn;
    }

    X86SimdTwoPieceHashContextDestroy(&context);
}

// ============ X86SimdTwoPieceHashHash + X86SimdTwoPieceHashUnhash ============

// Verifies that the SIMD variable-turn hash correctly integrates the turn bit
// and that `X86SimdTwoPieceHashGetTurn` perfectly recovers it.
TEST(X86SimdTwoPieceHashTest, SimdTurnEncodingAndRecovery) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    // Arbitrary valid pattern: 2 X's, 1 O
    uint64_t x_pattern = (1ULL << 0) | (1ULL << 8);
    uint64_t o_pattern = (1ULL << 1);

    // Note: _mm_set_epi64x takes (high, low), so lane 1 is O, lane 0 is X
    __m128i board = _mm_set_epi64x(o_pattern, x_pattern);

    // Hash the exact same board state with both turns
    Position hash_turn_0 = X86SimdTwoPieceHashHash(&context, board, 0);
    Position hash_turn_1 = X86SimdTwoPieceHashHash(&context, board, 1);

    EXPECT_NE(hash_turn_0, hash_turn_1)
        << "Hashes for the exact same board state but different turns must be "
           "distinct.";

    // Recover and verify turns
    int recovered_turn_0 = X86SimdTwoPieceHashGetTurn(hash_turn_0);
    int recovered_turn_1 = X86SimdTwoPieceHashGetTurn(hash_turn_1);

    EXPECT_EQ(recovered_turn_0, 0)
        << "Failed to correctly extract turn 0 from hash.";
    EXPECT_EQ(recovered_turn_1, 1)
        << "Failed to correctly extract turn 1 from hash.";

    // Unhash and verify lane integrity
    __m128i unhashed_0 = X86SimdTwoPieceHashUnhash(&context, hash_turn_0, 2, 1);
    EXPECT_EQ(_mm_extract_epi64(unhashed_0, 0), x_pattern)
        << "X pattern corrupted on turn 0 unhash.";
    EXPECT_EQ(_mm_extract_epi64(unhashed_0, 1), o_pattern)
        << "O pattern corrupted on turn 0 unhash.";

    __m128i unhashed_1 = X86SimdTwoPieceHashUnhash(&context, hash_turn_1, 2, 1);
    EXPECT_EQ(_mm_extract_epi64(unhashed_1, 0), x_pattern)
        << "X pattern corrupted on turn 1 unhash.";
    EXPECT_EQ(_mm_extract_epi64(unhashed_1, 1), o_pattern)
        << "O pattern corrupted on turn 1 unhash.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Ensures the SIMD variable-turn functions are strictly equivalent to
// their memory-based counterparts.
TEST(X86SimdTwoPieceHashTest, SimdVsMemoryApiEquivalence) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    uint64_t x_pattern = (1ULL << 3) | (1ULL << 5);
    uint64_t o_pattern = (1ULL << 2);

    // Memory representation
    uint64_t patterns[2] = {x_pattern, o_pattern};
    // SIMD representation
    __m128i board = _mm_set_epi64x(o_pattern, x_pattern);

    int turn = 1;

    // Compare Hash outputs
    Position mem_hash = X86SimdTwoPieceHashHashMem(&context, patterns, turn);
    Position simd_hash = X86SimdTwoPieceHashHash(&context, board, turn);

    EXPECT_EQ(mem_hash, simd_hash) << "SIMD and Mem Hash functions produced "
                                      "different hashes for identical states.";

    // Compare Unhash outputs
    uint64_t unhashed_patterns[2] = {0, 0};
    X86SimdTwoPieceHashUnhashMem(&context, simd_hash, 2, 1, unhashed_patterns);

    __m128i unhashed_simd =
        X86SimdTwoPieceHashUnhash(&context, simd_hash, 2, 1);

    EXPECT_EQ(unhashed_patterns[0], _mm_extract_epi64(unhashed_simd, 0))
        << "Mismatch in X lane between SIMD and Mem unhash.";
    EXPECT_EQ(unhashed_patterns[1], _mm_extract_epi64(unhashed_simd, 1))
        << "Mismatch in O lane between SIMD and Mem unhash.";

    X86SimdTwoPieceHashContextDestroy(&context);
}

// Ensures the round-trip bijection holds flawlessly across the entire
// doubled state space using the SIMD pipeline.
TEST(X86SimdTwoPieceHashTest, ExhaustiveSimdTurnRoundTripBijection) {
    X86SimdTwoPieceHashContext context;
    ASSERT_EQ(X86SimdTwoPieceHashContextInit(&context, 3, 3), kSuccess)
        << "Setup: Context must initialize successfully.";

    int num_x = 2;
    int num_o = 1;

    // The variable-turn state space is exactly double the fixed-turn state
    // space
    int64_t max_fixed_positions =
        X86SimdTwoPieceHashGetNumPositionsFixedTurn(&context, num_x, num_o);
    int64_t max_turned_positions = max_fixed_positions * 2;

    for (Position original_hash = 0; original_hash < max_turned_positions;
         ++original_hash) {
        // 1. Extract the turn
        int turn = X86SimdTwoPieceHashGetTurn(original_hash);
        ASSERT_TRUE(turn == 0 || turn == 1)
            << "Recovered turn must be strictly 0 or 1. Got: " << turn
            << " at hash " << original_hash;

        // 2. Unhash the board state to a SIMD register
        __m128i unhashed_board =
            X86SimdTwoPieceHashUnhash(&context, original_hash, num_x, num_o);

        // 3. Immediately re-hash the register alongside the extracted turn
        Position rehashed =
            X86SimdTwoPieceHashHash(&context, unhashed_board, turn);

        // 4. Verify absolute bijection
        ASSERT_EQ(rehashed, original_hash)
            << "SIMD variable-turn round trip failed! Original Hash: "
            << original_hash << ", Rehashed: " << rehashed
            << ", Recovered Turn: " << turn;
    }

    X86SimdTwoPieceHashContextDestroy(&context);
}

// ======================== X86SimdTwoPieceHashFlipDiag ========================

// Flipping completely empty or completely full boards should yield identical
// boards.
TEST(X86SimdTwoPieceHashFlipDiagTest, EmptyAndFullBoards) {
    uint64_t empty_pattern = 0ULL;
    uint64_t full_pattern = ~0ULL;  // All 1s

    __m128i board = _mm_set_epi64x(full_pattern, empty_pattern);
    __m128i flipped = X86SimdTwoPieceHashFlipDiag(board);

    EXPECT_EQ(_mm_extract_epi64(flipped, 0), empty_pattern)
        << "Empty board (lane 0) was altered during diagonal flip.";
    EXPECT_EQ(_mm_extract_epi64(flipped, 1), full_pattern)
        << "Full board (lane 1) was altered during diagonal flip.";
}

// Pieces strictly on the top-left to bottom-right diagonal must not move.
TEST(X86SimdTwoPieceHashFlipDiagTest, MainDiagonalInvariance) {
    // The main diagonal \ (0,0), (1,1), (2,2) ... (7,7)
    // 1 << 0 | 1 << 9 | 1 << 18 | 1 << 27 | 1 << 36 | 1 << 45 | 1 << 54 | 1 <<
    // 63
    constexpr uint64_t main_diag = 0x8040201008040201ULL;

    // Put it in both lanes for good measure
    __m128i board = _mm_set_epi64x(main_diag, main_diag);
    __m128i flipped = X86SimdTwoPieceHashFlipDiag(board);

    EXPECT_EQ(_mm_extract_epi64(flipped, 0), main_diag)
        << "Main diagonal bits shifted improperly on lane 0.";
    EXPECT_EQ(_mm_extract_epi64(flipped, 1), main_diag)
        << "Main diagonal bits shifted improperly on lane 1.";
}

// Verifies (row, col) accurately maps to (col, row).
// Assuming standard bitboard layout: bit index = row * 8 + col
TEST(X86SimdTwoPieceHashFlipDiagTest, OffDiagonalTransposition) {
    // Lane 0: (0, 7) [bit 7] and (2, 5) [bit 21]
    constexpr uint64_t lane0_orig = (1ULL << 7) | (1ULL << 21);
    // Flipped to: (7, 0) [bit 56] and (5, 2) [bit 42]
    constexpr uint64_t lane0_expected = (1ULL << 56) | (1ULL << 42);

    // Lane 1: (1, 0) [bit 8] and (0, 2) [bit 2]
    constexpr uint64_t lane1_orig = (1ULL << 8) | (1ULL << 2);
    // Flipped to: (0, 1) [bit 1] and (2, 0) [bit 16]
    constexpr uint64_t lane1_expected = (1ULL << 1) | (1ULL << 16);

    __m128i board = _mm_set_epi64x(lane1_orig, lane0_orig);
    __m128i flipped = X86SimdTwoPieceHashFlipDiag(board);

    EXPECT_EQ(_mm_extract_epi64(flipped, 0), lane0_expected)
        << "Lane 0 asymmetric bits did not mirror correctly across the main "
           "diagonal.";
    EXPECT_EQ(_mm_extract_epi64(flipped, 1), lane1_expected)
        << "Lane 1 asymmetric bits did not mirror correctly across the main "
           "diagonal.";
}

// Flipping a board twice across the same diagonal must perfectly restore it.
TEST(X86SimdTwoPieceHashFlipDiagTest, DoubleFlipInvolution) {
    // Arbitrary complex patterns
    // e.g., upper triangle filled, checkerboard pattern
    constexpr uint64_t lane0_pattern = 0xAA55AA55AA55AA55ULL;  // Checkerboard
    constexpr uint64_t lane1_pattern = 0xF0F0F0F00F0F0F0FULL;  // 4x4 squares

    __m128i original_board = _mm_set_epi64x(lane1_pattern, lane0_pattern);

    __m128i flipped_once = X86SimdTwoPieceHashFlipDiag(original_board);
    __m128i flipped_twice = X86SimdTwoPieceHashFlipDiag(flipped_once);

    EXPECT_EQ(_mm_extract_epi64(flipped_twice, 0), lane0_pattern)
        << "Double flip (lane 0) failed to restore original pattern.";
    EXPECT_EQ(_mm_extract_epi64(flipped_twice, 1), lane1_pattern)
        << "Double flip (lane 1) failed to restore original pattern.";
}

// ====================== X86SimdTwoPieceHashFlipVertical ======================

// Flipping completely empty or completely full boards should yield identical
// boards.
TEST(X86SimdTwoPieceHashFlipVerticalTest, EmptyAndFullBoards) {
    uint64_t empty_pattern = 0ULL;
    uint64_t full_pattern = ~0ULL;  // All 1s

    __m128i board = _mm_set_epi64x(full_pattern, empty_pattern);

    // Test on a standard 8-row board
    __m128i flipped = X86SimdTwoPieceHashFlipVertical(board, 8);

    EXPECT_EQ(_mm_extract_epi64(flipped, 0), empty_pattern)
        << "Empty board (lane 0) was altered during vertical flip.";
    EXPECT_EQ(_mm_extract_epi64(flipped, 1), full_pattern)
        << "Full board (lane 1) was altered during vertical flip.";
}

// Verifies row swapping on a full 8-row board.
// Row 0 swaps with Row 7, Row 1 swaps with Row 6, etc.
TEST(X86SimdTwoPieceHashFlipVerticalTest, Standard8RowFlip) {
    // Lane 0: Bit at Row 0, Col 2 (bit 2) and Row 1, Col 5 (bit 13)
    uint64_t lane0_orig = (1ULL << 2) | (1ULL << 13);
    // Expected: Row 7, Col 2 (bit 58) and Row 6, Col 5 (bit 53)
    uint64_t lane0_expected = (1ULL << 58) | (1ULL << 53);

    // Lane 1: Bit at Row 7, Col 0 (bit 56) and Row 3, Col 7 (bit 31)
    uint64_t lane1_orig = (1ULL << 56) | (1ULL << 31);
    // Expected: Row 0, Col 0 (bit 0) and Row 4, Col 7 (bit 39)
    uint64_t lane1_expected = (1ULL << 0) | (1ULL << 39);

    __m128i board = _mm_set_epi64x(lane1_orig, lane0_orig);
    __m128i flipped = X86SimdTwoPieceHashFlipVertical(board, 8);

    EXPECT_EQ(_mm_extract_epi64(flipped, 0), lane0_expected)
        << "Lane 0 did not flip correctly across 8 rows.";
    EXPECT_EQ(_mm_extract_epi64(flipped, 1), lane1_expected)
        << "Lane 1 did not flip correctly across 8 rows.";
}

// Verifies that the 'rows' parameter correctly restricts the flip boundaries.
TEST(X86SimdTwoPieceHashFlipVerticalTest, PartialBoardFlip) {
    int rows = 3;

    // Original layout for 3 rows:
    // Row 0: Bit at Col 0 (bit 0)
    // Row 1: Bit at Col 1 (bit 9)
    // Row 2: Bit at Col 2 (bit 18)
    uint64_t pattern = (1ULL << 0) | (1ULL << 9) | (1ULL << 18);

    // Expected after 3-row flip:
    // Row 0 gets old Row 2 -> Col 2 (bit 2)
    // Row 1 stays old Row 1 -> Col 1 (bit 9)
    // Row 2 gets old Row 0 -> Col 0 (bit 16)
    uint64_t expected = (1ULL << 2) | (1ULL << 9) | (1ULL << 16);

    // Apply same pattern to both lanes
    __m128i board = _mm_set_epi64x(pattern, pattern);
    __m128i flipped = X86SimdTwoPieceHashFlipVertical(board, rows);

    EXPECT_EQ(_mm_extract_epi64(flipped, 0), expected)
        << "Lane 0 did not correctly vertically flip within a " << rows
        << "-row bounding box.";
    EXPECT_EQ(_mm_extract_epi64(flipped, 1), expected)
        << "Lane 1 did not correctly vertically flip within a " << rows
        << "-row bounding box.";
}

// Flipping a board twice vertically (with the same row count) must completely
// restore it.
TEST(X86SimdTwoPieceHashFlipVerticalTest, DoubleFlipInvolution) {
    // Arbitrary complex patterns (e.g., checkerboard and clustered bits)
    uint64_t lane0_pattern = 0xAA55AA55AA55AA55ULL;
    uint64_t lane1_pattern = 0x0F0F0F0F00FF00FFULL;

    __m128i original_board = _mm_set_epi64x(lane1_pattern, lane0_pattern);

    // Test involution on full 8 rows
    __m128i flipped_once_8 = X86SimdTwoPieceHashFlipVertical(original_board, 8);
    __m128i flipped_twice_8 =
        X86SimdTwoPieceHashFlipVertical(flipped_once_8, 8);

    EXPECT_EQ(_mm_extract_epi64(flipped_twice_8, 0), lane0_pattern)
        << "Double flip (lane 0, 8 rows) failed to restore original pattern.";
    EXPECT_EQ(_mm_extract_epi64(flipped_twice_8, 1), lane1_pattern)
        << "Double flip (lane 1, 8 rows) failed to restore original pattern.";

    // Test involution on partial 5 rows
    // Note: This relies on the bits outside the first 5 rows being gracefully
    // ignored or consistently shifted by the implementation.
    __m128i flipped_once_5 = X86SimdTwoPieceHashFlipVertical(original_board, 5);
    __m128i flipped_twice_5 =
        X86SimdTwoPieceHashFlipVertical(flipped_once_5, 5);

    // Depending on whether the function clears garbage bits above 'rows', we
    // only verify the bits within the 5-row bounding box (mask =
    // 0x000000FFFFFFFFFF)
    uint64_t row5_mask = 0x000000FFFFFFFFFFULL;

    EXPECT_EQ(_mm_extract_epi64(flipped_twice_5, 0) & row5_mask,
              lane0_pattern & row5_mask)
        << "Double flip (lane 0, 5 rows) failed to restore bounded original "
           "pattern.";
    EXPECT_EQ(_mm_extract_epi64(flipped_twice_5, 1) & row5_mask,
              lane1_pattern & row5_mask)
        << "Double flip (lane 1, 5 rows) failed to restore bounded original "
           "pattern.";
}

// ==================== X86SimdTwoPieceHashMirrorHorizontal ====================

// Mirroring completely empty or completely full boards across 8 columns
// should yield identical boards.
TEST(X86SimdTwoPieceHashMirrorHorizontalTest, EmptyAndFullBoards) {
    uint64_t empty_pattern = 0ULL;
    uint64_t full_pattern = ~0ULL;  // All 1s

    __m128i board = _mm_set_epi64x(full_pattern, empty_pattern);

    // Test on a standard 8-column board
    __m128i mirrored = X86SimdTwoPieceHashMirrorHorizontal(board, 8);

    EXPECT_EQ(_mm_extract_epi64(mirrored, 0), empty_pattern)
        << "Empty board (lane 0) was altered during horizontal mirror.";
    EXPECT_EQ(_mm_extract_epi64(mirrored, 1), full_pattern)
        << "Full board (lane 1) was altered during horizontal mirror.";
}

// Verifies column swapping on a full 8-column board.
// Col 0 swaps with Col 7, Col 1 swaps with Col 6, etc.
TEST(X86SimdTwoPieceHashMirrorHorizontalTest, Standard8ColMirror) {
    // Lane 0: Bit at Row 0, Col 0 (bit 0) and Row 2, Col 1 (bit 17)
    uint64_t lane0_orig = (1ULL << 0) | (1ULL << 17);
    // Expected: Row 0, Col 7 (bit 7) and Row 2, Col 6 (bit 22)
    uint64_t lane0_expected = (1ULL << 7) | (1ULL << 22);

    // Lane 1: Bit at Row 7, Col 7 (bit 63) and Row 4, Col 3 (bit 35)
    uint64_t lane1_orig = (1ULL << 63) | (1ULL << 35);
    // Expected: Row 7, Col 0 (bit 56) and Row 4, Col 4 (bit 36)
    uint64_t lane1_expected = (1ULL << 56) | (1ULL << 36);

    __m128i board = _mm_set_epi64x(lane1_orig, lane0_orig);
    __m128i mirrored = X86SimdTwoPieceHashMirrorHorizontal(board, 8);

    EXPECT_EQ(_mm_extract_epi64(mirrored, 0), lane0_expected)
        << "Lane 0 did not mirror correctly across 8 columns.";
    EXPECT_EQ(_mm_extract_epi64(mirrored, 1), lane1_expected)
        << "Lane 1 did not mirror correctly across 8 columns.";
}

// Verifies that the 'cols' parameter correctly restricts the mirror boundaries.
TEST(X86SimdTwoPieceHashMirrorHorizontalTest, PartialBoardMirror) {
    int cols = 5;

    // Original layout for 5 cols:
    // Row 1: Bit at Col 0 (bit 8)
    // Row 3: Bit at Col 2 (bit 26) - center column for 5 cols, should not move
    uint64_t pattern = (1ULL << 8) | (1ULL << 26);

    // Expected after 5-col mirror:
    // Row 1 gets old Col 0 -> Col 4 (bit 12)
    // Row 3 stays old Col 2 -> Col 2 (bit 26)
    uint64_t expected = (1ULL << 12) | (1ULL << 26);

    // Apply same pattern to both lanes
    __m128i board = _mm_set_epi64x(pattern, pattern);
    __m128i mirrored = X86SimdTwoPieceHashMirrorHorizontal(board, cols);

    EXPECT_EQ(_mm_extract_epi64(mirrored, 0), expected)
        << "Lane 0 did not correctly horizontally mirror within a " << cols
        << "-col bounding box.";
    EXPECT_EQ(_mm_extract_epi64(mirrored, 1), expected)
        << "Lane 1 did not correctly horizontally mirror within a " << cols
        << "-col bounding box.";
}

// Mirroring a board twice horizontally (with the same column count) must
// completely restore it.
TEST(X86SimdTwoPieceHashMirrorHorizontalTest, DoubleMirrorInvolution) {
    // Arbitrary complex patterns
    uint64_t lane0_pattern = 0x123456789ABCDEF0ULL;
    uint64_t lane1_pattern = 0x0F0F0F0F00FF00FFULL;

    __m128i original_board = _mm_set_epi64x(lane1_pattern, lane0_pattern);

    // Test involution on full 8 columns
    __m128i mirrored_once_8 =
        X86SimdTwoPieceHashMirrorHorizontal(original_board, 8);
    __m128i mirrored_twice_8 =
        X86SimdTwoPieceHashMirrorHorizontal(mirrored_once_8, 8);

    EXPECT_EQ(_mm_extract_epi64(mirrored_twice_8, 0), lane0_pattern)
        << "Double mirror (lane 0, 8 cols) failed to restore original pattern.";
    EXPECT_EQ(_mm_extract_epi64(mirrored_twice_8, 1), lane1_pattern)
        << "Double mirror (lane 1, 8 cols) failed to restore original pattern.";

    // Test involution on partial 5 columns
    __m128i mirrored_once_5 =
        X86SimdTwoPieceHashMirrorHorizontal(original_board, 5);
    __m128i mirrored_twice_5 =
        X86SimdTwoPieceHashMirrorHorizontal(mirrored_once_5, 5);

    // Mask for 5 columns per row: 0x1F (0b00011111) repeated across 8 rows
    uint64_t col5_mask = 0x1F1F1F1F1F1F1F1FULL;

    EXPECT_EQ(_mm_extract_epi64(mirrored_twice_5, 0) & col5_mask,
              lane0_pattern & col5_mask)
        << "Double mirror (lane 0, 5 cols) failed to restore bounded original "
           "pattern.";
    EXPECT_EQ(_mm_extract_epi64(mirrored_twice_5, 1) & col5_mask,
              lane1_pattern & col5_mask)
        << "Double mirror (lane 1, 5 cols) failed to restore bounded original "
           "pattern.";
}

// ======================= X86SimdTwoPieceHashSwapPieces =======================

// Verifies that distinct piece layouts for X and O perfectly trade places.
TEST(X86SimdTwoPieceHashTest, BasicSwapPieces) {
    // Arbitrary distinct patterns representing piece locations
    uint64_t x_pieces_orig = 0xAAAAAAAAAAAAAAAAULL;  // Lane 0
    uint64_t o_pieces_orig = 0x5555555555555555ULL;  // Lane 1

    // _mm_set_epi64x takes (lane1, lane0)
    __m128i board = _mm_set_epi64x(o_pieces_orig, x_pieces_orig);
    __m128i swapped = X86SimdTwoPieceHashSwapPieces(board);

    // After swapping, Lane 0 should have O's original pieces,
    // and Lane 1 should have X's original pieces.
    EXPECT_EQ(_mm_extract_epi64(swapped, 0), o_pieces_orig)
        << "Lane 0 did not receive Lane 1's pieces.";
    EXPECT_EQ(_mm_extract_epi64(swapped, 1), x_pieces_orig)
        << "Lane 1 did not receive Lane 0's pieces.";
}

// Tests swapping when one player has pieces everywhere and the other has none.
TEST(X86SimdTwoPieceHashTest, SwapEmptyAndFull) {
    uint64_t empty = 0ULL;
    uint64_t full = ~0ULL;  // All 1s

    // X has no pieces (empty), O fills the board (full)
    __m128i board = _mm_set_epi64x(full, empty);
    __m128i swapped = X86SimdTwoPieceHashSwapPieces(board);

    // After swapping, X should be full, O should be empty
    EXPECT_EQ(_mm_extract_epi64(swapped, 0), full)
        << "Empty lane failed to receive the full bitboard.";
    EXPECT_EQ(_mm_extract_epi64(swapped, 1), empty)
        << "Full lane failed to receive the empty bitboard.";
}

// Swapping pieces twice must return the exact original piece configuration.
TEST(X86SimdTwoPieceHashTest, DoubleSwapPiecesInvolution) {
    uint64_t x_pieces_orig = 0x123456789ABCDEF0ULL;
    uint64_t o_pieces_orig = 0x0FEDCBA987654321ULL;

    __m128i original_board = _mm_set_epi64x(o_pieces_orig, x_pieces_orig);

    __m128i swapped_once = X86SimdTwoPieceHashSwapPieces(original_board);
    __m128i swapped_twice = X86SimdTwoPieceHashSwapPieces(swapped_once);

    EXPECT_EQ(_mm_extract_epi64(swapped_twice, 0), x_pieces_orig)
        << "Double swap failed to restore X's original bitboard in Lane 0.";
    EXPECT_EQ(_mm_extract_epi64(swapped_twice, 1), o_pieces_orig)
        << "Double swap failed to restore O's original bitboard in Lane 1.";
}

// ===================== X86SimdTwoPieceHashBoardLessThan =====================

// Verifies that comparing two identical boards returns false, as the
// comparison evaluates if 'a' is *strictly* less than 'b'.
TEST(X86SimdTwoPieceHashBoardLessThanTest, EqualBoards) {
    uint64_t lane1 = 0x00AABBCCDDEEFF11ULL;
    uint64_t lane0 = 0x0011223344556677ULL;

    __m128i board = _mm_set_epi64x(lane1, lane0);

    EXPECT_FALSE(X86SimdTwoPieceHashBoardLessThan(board, board))
        << "Identical boards must not evaluate to strictly less than.";
}

// Verifies that if the upper 64 bits (Lane 1) of board A are smaller
// than board B, A is considered strictly less than B, even if A's
// lower 64 bits (Lane 0) are maximized.
TEST(X86SimdTwoPieceHashBoardLessThanTest, Lane1Dominates) {
    uint64_t a_lane1 = 0x0000000000000005ULL;
    // Maximize Lane 0 to ensure Lane 1 dominates the comparison logic
    uint64_t a_lane0 = 0x00FFFFFFFFFFFFFFULL;
    __m128i board_a = _mm_set_epi64x(a_lane1, a_lane0);

    uint64_t b_lane1 = 0x0000000000000006ULL;
    uint64_t b_lane0 = 0x0000000000000000ULL;
    __m128i board_b = _mm_set_epi64x(b_lane1, b_lane0);

    // a < b should be true because Lane 1 is smaller
    EXPECT_TRUE(X86SimdTwoPieceHashBoardLessThan(board_a, board_b))
        << "Board A should be strictly less than Board B based on Lane 1.";

    // b < a should be false
    EXPECT_FALSE(X86SimdTwoPieceHashBoardLessThan(board_b, board_a))
        << "Board B is greater than Board A, but evaluated to less than.";
}

// Verifies that when the upper 64 bits (Lane 1) are identical, the
// comparison correctly falls back to evaluating the lower 64 bits (Lane 0).
TEST(X86SimdTwoPieceHashBoardLessThanTest, Lane0Tiebreaker) {
    uint64_t shared_lane1 = 0x00123456789ABCDEULL;

    uint64_t a_lane0 = 0x000000000000004FULL;  // Smaller
    __m128i board_a = _mm_set_epi64x(shared_lane1, a_lane0);

    uint64_t b_lane0 = 0x0000000000000050ULL;  // Larger
    __m128i board_b = _mm_set_epi64x(shared_lane1, b_lane0);

    // a < b should be true
    EXPECT_TRUE(X86SimdTwoPieceHashBoardLessThan(board_a, board_b))
        << "Board A should be strictly less than Board B based on Lane 0 "
           "tiebreaker.";

    // b < a should be false
    EXPECT_FALSE(X86SimdTwoPieceHashBoardLessThan(board_b, board_a))
        << "Board B is greater than Board A (Lane 0 tiebreaker), but evaluated "
           "to less than.";
}

// Verifies that boards utilizing the maximum allowed capacity for 7 rows
// (where the 55th bit is set, but the 8th byte is entirely zero) correctly
// compare without triggering signed-integer evaluation artifacts.
TEST(X86SimdTwoPieceHashBoardLessThanTest, Max7RowCapacitySafe) {
    // 0x00FFFFFFFFFFFFFF sets all bits in the first 7 rows,
    // but leaves the 8th row (highest byte) completely zero.
    uint64_t max_7row_val = 0x00FFFFFFFFFFFFFFULL;
    uint64_t slightly_less = 0x00FFFFFFFFFFFFFEULL;

    __m128i max_board = _mm_set_epi64x(max_7row_val, max_7row_val);
    __m128i lesser_board = _mm_set_epi64x(max_7row_val, slightly_less);

    EXPECT_TRUE(X86SimdTwoPieceHashBoardLessThan(lesser_board, max_board))
        << "Comparison failed near the 7-row maximum boundary.";

    EXPECT_FALSE(X86SimdTwoPieceHashBoardLessThan(max_board, lesser_board))
        << "Inverse comparison failed near the 7-row maximum boundary.";
}

// ======================== X86SimdTwoPieceHashMinBoard ========================

// Verifies that min(board, board) == board
TEST(X86SimdTwoPieceHashMinBoardTest, EqualBoards) {
    uint64_t lane1 = 0x00AABBCCDDEEFF11ULL;
    uint64_t lane0 = 0x0011223344556677ULL;

    __m128i board = _mm_set_epi64x(lane1, lane0);
    __m128i result = X86SimdTwoPieceHashMinBoard(board, board);

    EXPECT_EQ(_mm_extract_epi64(result, 0), lane0)
        << "Min of identical boards returned incorrect Lane 0.";
    EXPECT_EQ(_mm_extract_epi64(result, 1), lane1)
        << "Min of identical boards returned incorrect Lane 1.";
}

// Verifies that if Lane 1 is smaller, that board is the minimum, even if its
// Lane 0 is much larger.
TEST(X86SimdTwoPieceHashMinBoardTest, Lane1Dominates) {
    // Board A has a smaller Lane 1, but a maximized Lane 0
    uint64_t a_lane1 = 0x0000000000000005ULL;
    uint64_t a_lane0 = 0x00FFFFFFFFFFFFFFULL;
    __m128i board_a = _mm_set_epi64x(a_lane1, a_lane0);

    // Board B has a larger Lane 1, but a minimized Lane 0
    uint64_t b_lane1 = 0x0000000000000006ULL;
    uint64_t b_lane0 = 0x0000000000000000ULL;
    __m128i board_b = _mm_set_epi64x(b_lane1, b_lane0);

    __m128i result = X86SimdTwoPieceHashMinBoard(board_a, board_b);

    // Board A should be selected because 5 < 6
    EXPECT_EQ(_mm_extract_epi64(result, 0), a_lane0)
        << "Lane 1 domination failed: Incorrect Lane 0 selected.";
    EXPECT_EQ(_mm_extract_epi64(result, 1), a_lane1)
        << "Lane 1 domination failed: Incorrect Lane 1 selected.";
}

// Verifies that if Lane 1 is equal, Lane 0 determines the minimum.
TEST(X86SimdTwoPieceHashMinBoardTest, Lane0Tiebreaker) {
    uint64_t shared_lane1 = 0x00123456789ABCDEULL;

    uint64_t a_lane0 = 0x0000000000000050ULL;  // Larger
    __m128i board_a = _mm_set_epi64x(shared_lane1, a_lane0);

    uint64_t b_lane0 = 0x000000000000004FULL;  // Smaller
    __m128i board_b = _mm_set_epi64x(shared_lane1, b_lane0);

    __m128i result = X86SimdTwoPieceHashMinBoard(board_a, board_b);

    // Board B should be selected because 0x4F < 0x50
    EXPECT_EQ(_mm_extract_epi64(result, 0), b_lane0)
        << "Lane 0 tiebreaker failed: Selected the larger board.";
    EXPECT_EQ(_mm_extract_epi64(result, 1), shared_lane1)
        << "Lane 0 tiebreaker failed: Corrupted shared Lane 1.";
}

// Verifies that min(a, b) == min(b, a).
TEST(X86SimdTwoPieceHashMinBoardTest, ArgumentSymmetry) {
    __m128i board_a =
        _mm_set_epi64x(0x0000000000000010ULL, 0x0000000000000000ULL);
    __m128i board_b =
        _mm_set_epi64x(0x0000000000000020ULL, 0x00FFFFFFFFFFFFFFULL);

    __m128i result_ab = X86SimdTwoPieceHashMinBoard(board_a, board_b);
    __m128i result_ba = X86SimdTwoPieceHashMinBoard(board_b, board_a);

    // Both should yield board_a (Lane 1: 0x10 < 0x20)
    EXPECT_EQ(_mm_extract_epi64(result_ab, 0), _mm_extract_epi64(result_ba, 0))
        << "min(a, b) and min(b, a) returned different Lane 0s.";
    EXPECT_EQ(_mm_extract_epi64(result_ab, 1), _mm_extract_epi64(result_ba, 1))
        << "min(a, b) and min(b, a) returned different Lane 1s.";

    EXPECT_EQ(_mm_extract_epi64(result_ab, 1), 0x0000000000000010ULL)
        << "Symmetry test selected the maximum instead of the minimum.";
}

// ================================ cmplt_u128 ================================

// Verifies that comparing two identical 128-bit integers returns false,
// as the comparison checks for strictly less than.
TEST(CmpltU128Test, EqualIntegers) {
    uint64_t lane1 = 0xDEADBEEFCAFEBABEULL;
    uint64_t lane0 = 0x8BADF00D0D15EA5EULL;

    __m128i val = _mm_set_epi64x(lane1, lane0);

    EXPECT_FALSE(cmplt_u128(val, val)) << "Identical 128-bit integers must not "
                                          "evaluate to strictly less than.";
}

// Verifies that the upper 64 bits (Lane 1) strictly dominate the comparison,
// meaning a smaller Lane 1 guarantees a smaller 128-bit integer regardless
// of the contents of Lane 0.
TEST(CmpltU128Test, Upper64BitsDominate) {
    uint64_t a_lane1 = 0x0000000000000005ULL;
    // Maximize Lane 0 for 'a' to ensure Lane 1 dominates the logic
    uint64_t a_lane0 = 0xFFFFFFFFFFFFFFFFULL;
    __m128i val_a = _mm_set_epi64x(a_lane1, a_lane0);

    uint64_t b_lane1 = 0x0000000000000006ULL;
    uint64_t b_lane0 = 0x0000000000000000ULL;
    __m128i val_b = _mm_set_epi64x(b_lane1, b_lane0);

    EXPECT_TRUE(cmplt_u128(val_a, val_b))
        << "Value A should be strictly less than Value B based on Lane 1.";

    EXPECT_FALSE(cmplt_u128(val_b, val_a))
        << "Value B is greater than Value A, but evaluated to less than.";
}

// Verifies that when the upper 64 bits (Lane 1) are equal, the lower
// 64 bits (Lane 0) correctly act as the tiebreaker.
TEST(CmpltU128Test, Lower64BitsTiebreaker) {
    uint64_t shared_lane1 = 0x123456789ABCDEF0ULL;

    uint64_t a_lane0 = 0x000000000000004FULL;
    __m128i val_a = _mm_set_epi64x(shared_lane1, a_lane0);

    uint64_t b_lane0 = 0x0000000000000050ULL;
    __m128i val_b = _mm_set_epi64x(shared_lane1, b_lane0);

    EXPECT_TRUE(cmplt_u128(val_a, val_b))
        << "Value A should be strictly less than Value B based on Lane 0 "
           "tiebreaker.";

    EXPECT_FALSE(cmplt_u128(val_b, val_a))
        << "Value B is greater than Value A (Lane 0 tiebreaker), but evaluated "
           "to less than.";
}

// Verifies that the most significant bit (MSB) of the upper 64 bits
// (Lane 1) is correctly treated as part of an unsigned integer, rather
// than a sign bit that would make the number negative (and incorrectly
// smaller).
TEST(CmpltU128Test, UnsignedEvaluationLane1) {
    // 0x7FFFFFFFFFFFFFFF has the MSB set to 0
    uint64_t a_lane1 = 0x7FFFFFFFFFFFFFFFULL;
    // 0x8000000000000000 has the MSB set to 1
    uint64_t b_lane1 = 0x8000000000000000ULL;

    __m128i val_a = _mm_set_epi64x(a_lane1, 0ULL);
    __m128i val_b = _mm_set_epi64x(b_lane1, 0ULL);

    // If evaluated as signed, 0x80... is negative and thus "less than" 0x7F...
    // Unsigned logic must evaluate 0x7F... < 0x80... as true.
    EXPECT_TRUE(cmplt_u128(val_a, val_b))
        << "Unsigned comparison failed: MSB in Lane 1 was treated as a sign "
           "bit.";

    EXPECT_FALSE(cmplt_u128(val_b, val_a))
        << "Unsigned comparison failed: Negative-interpreted Lane 1 was "
           "considered smaller.";
}

// Verifies that the most significant bit (MSB) of the lower 64 bits
// (Lane 0) is correctly treated as an unsigned magnitude during a tiebreaker,
// rather than a sign bit.
TEST(CmpltU128Test, UnsignedEvaluationLane0) {
    uint64_t shared_lane1 = 0x0ULL;

    uint64_t a_lane0 = 0x7FFFFFFFFFFFFFFFULL;
    uint64_t b_lane0 = 0x8000000000000000ULL;

    __m128i val_a = _mm_set_epi64x(shared_lane1, a_lane0);
    __m128i val_b = _mm_set_epi64x(shared_lane1, b_lane0);

    // If evaluated as signed, 0x80... is negative and thus "less than" 0x7F...
    // Unsigned logic must evaluate 0x7F... < 0x80... as true.
    EXPECT_TRUE(cmplt_u128(val_a, val_b))
        << "Unsigned tiebreaker failed: MSB in Lane 0 was treated as a sign "
           "bit.";

    EXPECT_FALSE(cmplt_u128(val_b, val_a))
        << "Unsigned tiebreaker failed: Negative-interpreted Lane 0 was "
           "considered smaller.";
}

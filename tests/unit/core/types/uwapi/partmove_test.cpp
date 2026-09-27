/**
 * @file partmove_test.cpp
 * @brief Unit tests for Partmove.
 *
 * @copyright GamesCrafters research and development group
 * SPDX-License-Identifier: GPL-3.0-or-later
 * (see root COPYING file or accompanying source file)
 */

#include <gtest/gtest.h>

extern "C" {
#include "core/data_structures/cstring.h"
#include "core/types/uwapi/partmove.h"
}

// Verifies that PartMoveDestroy correctly cleans up a Partmove with all fields
// populated.
TEST(PartmoveTest, DestroyPopulatedPartmove) {
    Partmove pm;
    ASSERT_TRUE(CStringInitCopyCharArray(&pm.autogui_move, "auto_move"));
    ASSERT_TRUE(CStringInitCopyCharArray(&pm.formal_move, "formal_move"));
    ASSERT_TRUE(CStringInitCopyCharArray(&pm.from, "from_pos"));
    ASSERT_TRUE(CStringInitCopyCharArray(&pm.to, "to_pos"));
    ASSERT_TRUE(CStringInitCopyCharArray(&pm.full, "full_move"));

    PartMoveDestroy(&pm);

    EXPECT_TRUE(CStringIsNull(&pm.autogui_move));
    EXPECT_TRUE(CStringIsNull(&pm.formal_move));
    EXPECT_TRUE(CStringIsNull(&pm.from));
    EXPECT_TRUE(CStringIsNull(&pm.to));
    EXPECT_TRUE(CStringIsNull(&pm.full));
}

// Verifies that PartMoveDestroy handles a Partmove with null CStrings.
TEST(PartmoveTest, DestroyNullStringsPartmove) {
    Partmove pm;
    pm.autogui_move = CStringGetNull();
    pm.formal_move = CStringGetNull();
    pm.from = CStringGetNull();
    pm.to = CStringGetNull();
    pm.full = CStringGetNull();

    PartMoveDestroy(&pm);

    EXPECT_TRUE(CStringIsNull(&pm.autogui_move));
    EXPECT_TRUE(CStringIsNull(&pm.formal_move));
    EXPECT_TRUE(CStringIsNull(&pm.from));
    EXPECT_TRUE(CStringIsNull(&pm.to));
    EXPECT_TRUE(CStringIsNull(&pm.full));
}

// Verifies that PartMoveDestroy handles a Partmove with some empty strings.
TEST(PartmoveTest, DestroyMixedStringsPartmove) {
    Partmove pm;
    ASSERT_TRUE(CStringInitEmpty(&pm.autogui_move));
    pm.formal_move = CStringGetNull();
    ASSERT_TRUE(CStringInitCopyCharArray(&pm.from, "from_pos"));
    ASSERT_TRUE(CStringInitEmpty(&pm.to));
    pm.full = CStringGetNull();

    PartMoveDestroy(&pm);

    EXPECT_TRUE(CStringIsNull(&pm.autogui_move));
    EXPECT_TRUE(CStringIsNull(&pm.formal_move));
    EXPECT_TRUE(CStringIsNull(&pm.from));
    EXPECT_TRUE(CStringIsNull(&pm.to));
    EXPECT_TRUE(CStringIsNull(&pm.full));
}

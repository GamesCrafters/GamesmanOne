/**
 * @file winkers.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of Winkers the game.
 * @details https://boardgamegeek.com/boardgame/22057/winkers-the-game
 * @version 1.0.0
 * @date 2024-12-23
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "games/winkers/winkers.h"

#include <assert.h>   // assert
#include <ctype.h>    // toupper
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t, int8_t
#include <stdio.h>    // sprintf
#include <stdlib.h>   // atoi
#include <string.h>   // strlen

#include "core/constants.h"
#include "core/generic_hash/generic_hash.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

// =================================== Types ===================================

typedef union {
    struct {
        int8_t winks[2];    /**< [0, 10] each, number of winks placed. */
        int8_t neutrals[2]; /**< [0, 10] each, number of neutrals placed. */
    } placed;
    Tier hash;
} WinkersTier;

// ================================= Constants =================================

enum {
    kBoardSize = 19,
    kNumSymmetries = 12,
};

static const WinkersTier kWinkersTierInit = {
    .placed = {.winks = {0, 0}, .neutrals = {0, 0}},
};

static const char kTurnToWink[] = {'-', 'X', 'O'};

static const int kThreeInARows[27][3] = {
    {0, 1, 2},   {3, 4, 5},    {4, 5, 6},    {7, 8, 9},    {8, 9, 10},
    {9, 10, 11}, {12, 13, 14}, {13, 14, 15}, {16, 17, 18},

    {2, 6, 11},  {1, 5, 10},   {5, 10, 15},  {0, 4, 9},    {4, 9, 14},
    {9, 14, 18}, {3, 8, 13},   {8, 13, 17},  {7, 12, 16},

    {0, 3, 7},   {1, 4, 8},    {4, 8, 12},   {2, 5, 9},    {5, 9, 13},
    {9, 13, 16}, {6, 10, 14},  {10, 14, 17}, {11, 15, 18},
};

// clang-format off

static const int8_t kSymmetryMatrix[kNumSymmetries][kBoardSize] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18},  // original
    {7, 3, 0, 12, 8, 4, 1, 16, 13, 9, 5, 2, 17, 14, 10, 6, 18, 15, 11},  // cw60
    {16, 12, 7, 17, 13, 8, 3, 18, 14, 9, 4, 0, 15, 10, 5, 1, 11, 6, 2},  // cw120
    {18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},  // 180
    {11, 15, 18, 6, 10, 14, 17, 2, 5, 9, 13, 16, 1, 4, 8, 12, 0, 3, 7},  // cw240
    {2, 6, 11, 1, 5, 10, 15, 0, 4, 9, 14, 18, 3, 8, 13, 17, 7, 12, 16},  // cw300

    {2, 1, 0, 6, 5, 4, 3, 11, 10, 9, 8, 7, 15, 14, 13, 12, 18, 17, 16},  // reflect
    {11, 6, 2, 15, 10, 5, 1, 18, 14, 9, 4, 0, 17, 13, 8, 3, 16, 12, 7},  // rcw60
    {18, 15, 11, 17, 14, 10, 6, 16, 13, 9, 5, 2, 12, 8, 4, 1, 7, 3, 0},  // rcw120
    {16, 17, 18, 12, 13, 14, 15, 7, 8, 9, 10, 11, 3, 4, 5, 6, 0, 1, 2},  // r180
    {7, 12, 16, 3, 8, 13, 17, 0, 4, 9, 14, 18, 1, 5, 10, 15, 2, 6, 11},  // rcw240
    {0, 3, 7, 1, 4, 8, 12, 2, 5, 9, 13, 16, 6, 10, 14, 17, 11, 15, 18},  // rcw300
};

// clang-format on

// ============================= Variant Settings =============================

// ============================= kWinkersSolverApi =============================

static Tier WinkersGetInitialTier(void) { return kWinkersTierInit.hash; }

static Position WinkersGetInitialPosition(void) {
    static const char *kInitialBoard = "-------------------";

    return GenericHashHashLabel(kWinkersTierInit.hash, kInitialBoard, 1);
}

static int64_t WinkersGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static MoveArray WinkersGenerateMovesInternal(WinkersTier t,
                                              char board[static kBoardSize],
                                              int turn) {
    MoveArray ret;
    MoveArrayInit(&ret);
    Move move;
    for (move = 0; move < kBoardSize; ++move) {
        bool can_add_neutral =
            (board[move] == '-' && t.placed.neutrals[turn - 1] < 10);
        bool can_add_wink =
            (board[move] == 'C' && t.placed.winks[turn - 1] < 10);
        if (can_add_neutral || can_add_wink) MoveArrayAppend(&ret, move);
    }

    return ret;
}

static MoveArray WinkersGenerateMoves(TierPosition tier_position) {
    // Unhash
    char board[kBoardSize];
    WinkersTier t = {.hash = tier_position.tier};
    Position pos = tier_position.position;
    bool success = GenericHashUnhashLabel(t.hash, pos, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurnLabel(t.hash, pos);

    return WinkersGenerateMovesInternal(t, board, turn);
}

static bool HasThreeInARow(char board[static kBoardSize], char face) {
    for (int i = 0; i < 27; ++i) {
        bool found = true;
        for (int j = 0; j < 3; ++j) {
            if (board[kThreeInARows[i][j]] != face) {
                found = false;
                break;
            }
        }
        if (found) return true;
    }

    return false;
}

static Value WinkersPrimitive(TierPosition tier_position) {
    // Unhash
    char board[kBoardSize];
    Position pos = tier_position.position;
    bool success = GenericHashUnhashLabel(tier_position.tier, pos, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurnLabel(tier_position.tier, pos);

    // If there is a three-in-a-row of the opponent's winkers, then the current
    // player has lost. Note that it is not possible for the current player to
    // to have three of their winkers in a row because otherwise the game should
    // have been over two turns ago.
    if (HasThreeInARow(board, kTurnToWink[3 - turn])) return kLose;

    // If the board is not full of winkers...
    for (int i = 0; i < kBoardSize; ++i) {
        if (board[i] != 'X' && board[i] != 'O') {
            // Check for the edge case where the current player has no moves.
            // This is defined as a winning position for the current player.
            MoveArray moves = WinkersGenerateMoves(tier_position);
            Value ret = moves.size == 0 ? kWin : kUndecided;
            MoveArrayDestroy(&moves);
            return ret;
        }
    }

    // Otherwise, the game is a tie.
    return kTie;
}

// board may be modified but is guaranteed to be restored upon returning.
static TierPosition WinkersDoMoveInternal(WinkersTier t,
                                          char board[static kBoardSize],
                                          int turn, Move move) {
    TierPosition ret;
    char backup = board[move];
    if (board[move] == '-') {
        // Placing a neutral piece
        assert(t.placed.neutrals[turn - 1] < 10);
        ++t.placed.neutrals[turn - 1];
        board[move] = 'C';
    } else {
        // Placing a wink
        assert(board[move] == 'C');
        assert(t.placed.winks[turn - 1] < 10);
        ++t.placed.winks[turn - 1];
        board[move] = kTurnToWink[turn];
    }
    ret.tier = t.hash;
    ret.position = GenericHashHashLabel(t.hash, board, 3 - turn);
    board[move] = backup;

    return ret;
}

static TierPosition WinkersDoMove(TierPosition tier_position, Move move) {
    // Unhash
    char board[kBoardSize];
    WinkersTier t = {.hash = tier_position.tier};
    Position pos = tier_position.position;
    bool success = GenericHashUnhashLabel(t.hash, pos, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurnLabel(t.hash, pos);

    return WinkersDoMoveInternal(t, board, turn, move);
}

static bool WinkersIsLegalPosition(TierPosition tier_position) {
    // Unhash
    char board[kBoardSize];
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    assert(success);
    (void)success;

    // It is not possible for both players to have three-in-a-rows at the same
    // time
    if (HasThreeInARow(board, 'X') && HasThreeInARow(board, 'O')) return false;

    return true;
}

// The following function relies on the assumption that characters with smaller
// ASCII values are also considered smaller by Generic Hash. This can be done by
// sorting the piece initialization arrays in descending ASCII order before
// calling GenericHashAddContext.
static_assert('-' < 'C' && 'C' < 'O' && 'O' < 'X', "");
static Position WinkersGetCanonicalPosition(TierPosition tier_position) {
    // Unhash
    Tier tier = tier_position.tier;
    Position pos = tier_position.position;
    char board[kBoardSize], symm_board[kBoardSize], min_board[kBoardSize];
    bool success = GenericHashUnhashLabel(tier, pos, board);
    assert(success);
    (void)success;
    memcpy(min_board, board, kBoardSize);
    int turn = GenericHashGetTurnLabel(tier, pos);

    for (int s = 1; s < kNumSymmetries; ++s) {
        for (int i = 0; i < kBoardSize; ++i) {
            symm_board[i] = board[kSymmetryMatrix[s][i]];
        }
        if (strncmp(symm_board, min_board, kBoardSize) < 0) {
            memcpy(min_board, symm_board, kBoardSize);
        }
    }

    return GenericHashHashLabel(tier, min_board, turn);
}

TierPositionArray WinkersGetCanonicalChildPositions(
    TierPosition tier_position) {
    // Unhash
    char board[kBoardSize];
    WinkersTier t = {.hash = tier_position.tier};
    Position pos = tier_position.position;
    bool success = GenericHashUnhashLabel(t.hash, pos, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurnLabel(t.hash, pos);

    MoveArray moves = WinkersGenerateMovesInternal(t, board, turn);
    TierPositionArray ret;
    TierPositionArrayInit(&ret);
    TierPositionHashSet dedup;
    TierPositionHashSetInit(&dedup, 0.5);
    for (int64_t i = 0; i < moves.size; ++i) {
        TierPosition child =
            WinkersDoMoveInternal(t, board, turn, moves.array[i]);
        child.position = WinkersGetCanonicalPosition(child);
        if (!TierPositionHashSetContains(&dedup, child)) {
            TierPositionHashSetAdd(&dedup, child);
            TierPositionArrayAppend(&ret, child);
        }
    }
    TierPositionHashSetDestroy(&dedup);
    MoveArrayDestroy(&moves);

    return ret;
}

static bool WinkersIsValidTier(Tier tier) {
    WinkersTier t = {.hash = tier};
    for (int i = 0; i < 2; ++i) {
        if (t.placed.neutrals[i] < 0) return false;
        if (t.placed.neutrals[i] > 10) return false;
        if (t.placed.winks[i] < 0) return false;
        if (t.placed.winks[i] > 10) return false;
    }
    int8_t neutrals_placed_total = t.placed.neutrals[0] + t.placed.neutrals[1];
    int8_t winks_placed_total = t.placed.winks[0] + t.placed.winks[1];
    if (neutrals_placed_total > kBoardSize) return false;
    if (winks_placed_total > neutrals_placed_total) return false;

    return true;
}

static int WinkersGetTurnFromTier(WinkersTier t) {
    return 1 + ((t.placed.winks[0] + t.placed.winks[1] + t.placed.neutrals[0] +
                 t.placed.neutrals[1]) &
                1);
}

static TierArray WinkersGetChildTiers(Tier tier) {
    WinkersTier t = {.hash = tier};
    int turn = WinkersGetTurnFromTier(t);
    TierArray ret;
    TierArrayInit(&ret);

    // Places a wink
    ++t.placed.winks[turn - 1];
    if (WinkersIsValidTier(t.hash)) TierArrayAppend(&ret, t.hash);
    --t.placed.winks[turn - 1];

    // Places a neutral checker
    ++t.placed.neutrals[turn - 1];
    if (WinkersIsValidTier(t.hash)) TierArrayAppend(&ret, t.hash);

    return ret;
}

TierType WinkersGetTierType(Tier tier) {
    (void)tier;
    return kTierTypeImmediateTransition;
}

static int WinkersGetTierName(Tier tier,
                              char name[static kDbFileNameLengthMax + 1]) {
    WinkersTier t = {.hash = tier};
    sprintf(name, "%dX_%dO_%dCX_%dCO", t.placed.winks[0], t.placed.winks[1],
            t.placed.neutrals[0], t.placed.neutrals[1]);

    return kNoError;
}

static const TierSolverApi kWinkersSolverApi = {
    .GetInitialTier = WinkersGetInitialTier,
    .GetInitialPosition = WinkersGetInitialPosition,
    .GetTierSize = WinkersGetTierSize,

    .GenerateMoves = WinkersGenerateMoves,
    .Primitive = WinkersPrimitive,
    .DoMove = WinkersDoMove,
    .IsLegalPosition = WinkersIsLegalPosition,
    .GetCanonicalPosition = WinkersGetCanonicalPosition,
    .GetCanonicalChildPositions = WinkersGetCanonicalChildPositions,

    .GetChildTiers = WinkersGetChildTiers,
    .GetTierType = WinkersGetTierType,
    .GetTierName = WinkersGetTierName,
};

// ============================ kWinkersGameplayApi ============================

static const char kWinkersPositionStringFormat[] =
    "            LEGEND                            TOTAL\n"
    "\n"
    "|       1     2     3        | :          %c     %c     %c\n"
    "|                            | :\n"
    "|    4     5     6     7     | :       %c     %c     %c     %c\n"
    "|                            | :\n"
    "| 8     9     10    11    12 | :    %c     %c     %c     %c     %c\n"
    "|                            | :\n"
    "|   13    14    15    16     | :       %c     %c     %c     %c\n"
    "|                            | :\n"
    "|       17    18    19       | :          %c     %c     %c\n"
    "\n"
    "Player 1 (X): %d neutral, %d wink(s) remaining\n"
    "Player 2 (O): %d neutral, %d wink(s) remaining";

// Simple automatic board string formatting from Winkers.
static int WinkersTierPositionToString(TierPosition tier_position,
                                       char *buffer) {
    // Unhash
    char board[kBoardSize];
    WinkersTier t = {.hash = tier_position.tier};
    Position pos = tier_position.position;
    bool success = GenericHashUnhashLabel(t.hash, pos, board);
    assert(success);
    (void)success;

    sprintf(buffer, kWinkersPositionStringFormat, board[0], board[1], board[2],
            board[3], board[4], board[5], board[6], board[7], board[8],
            board[9], board[10], board[11], board[12], board[13], board[14],
            board[15], board[16], board[17], board[18],
            10 - t.placed.neutrals[0], 10 - t.placed.winks[0],
            10 - t.placed.neutrals[1], 10 - t.placed.winks[1]);

    return kNoError;
}

static int WinkersMoveToString(Move move, char *buffer) {
    sprintf(buffer, "%d", (int)move + 1);

    return kNoError;
}

static bool WinkersIsValidMoveString(ReadOnlyString move_string) {
    int dest = atoi(move_string);
    return (dest >= 1 && dest <= kBoardSize);
}

static Move WinkersStringToMove(ReadOnlyString move_string) {
    return atoi(move_string) - 1;
}

static const GameplayApiCommon WinkersGameplayApiCommon = {
    .GetInitialPosition = WinkersGetInitialPosition,
    .position_string_length_max =
        sizeof(kWinkersPositionStringFormat) + 4 * kInt32Base10StringLengthMax,

    .move_string_length_max = 3,
    .MoveToString = WinkersMoveToString,

    .IsValidMoveString = WinkersIsValidMoveString,
    .StringToMove = WinkersStringToMove,
};

static const GameplayApiTier WinkersGameplayApiTier = {
    .GetInitialTier = WinkersGetInitialTier,

    .TierPositionToString = WinkersTierPositionToString,

    .GenerateMoves = WinkersGenerateMoves,
    .DoMove = WinkersDoMove,
    .Primitive = WinkersPrimitive,
};

static const GameplayApi kWinkersGameplayApi = {
    .common = &WinkersGameplayApiCommon,
    .tier = &WinkersGameplayApiTier,
};

// ========================= WinkersGetCurrentVariant =========================

// ========================== WinkersSetVariantOption ==========================

// ================================ WinkersInit ================================

static int WinkersInit(void *aux) {
    (void)aux;  // Unused.
    GenericHashReinitialize();
    WinkersTier t = kWinkersTierInit;
    int64_t total = 0, max = 0;
    int pieces[] = {'X', 0, 0, 'O', 0, 0, 'C', 0, 0, '-', 0, 0, -1};
    for (t.placed.winks[0] = 0; t.placed.winks[0] <= 10; ++t.placed.winks[0]) {
        for (t.placed.winks[1] = 0; t.placed.winks[1] <= 10;
             ++t.placed.winks[1]) {
            for (t.placed.neutrals[0] = 0; t.placed.neutrals[0] <= 10;
                 ++t.placed.neutrals[0]) {
                for (t.placed.neutrals[1] = 0; t.placed.neutrals[1] <= 10;
                     ++t.placed.neutrals[1]) {
                    if (!WinkersIsValidTier(t.hash)) continue;
                    pieces[1] = pieces[2] = t.placed.winks[0];
                    pieces[4] = pieces[5] = t.placed.winks[1];
                    pieces[7] = pieces[8] =
                        t.placed.neutrals[0] + t.placed.neutrals[1] -
                        t.placed.winks[0] - t.placed.winks[1];
                    pieces[10] = pieces[11] = kBoardSize -
                                              t.placed.neutrals[0] -
                                              t.placed.neutrals[1];
                    int turn = WinkersGetTurnFromTier(t);
                    bool success = GenericHashAddContext(turn, kBoardSize,
                                                         pieces, NULL, t.hash);
                    assert(success);
                    (void)success;
                    total += GenericHashNumPositionsLabel(t.hash);
                    if (max < GenericHashNumPositionsLabel(t.hash)) {
                        max = GenericHashNumPositionsLabel(t.hash);
                    }
                }
            }
        }
    }
    printf("total: %ld\nmax: %ld\n", total, max);

    return kNoError;
}

// ============================== WinkersFinalize ==============================

static int WinkersFinalize(void) { return kNoError; }

// =============================== kWinkersUwapi ===============================

// ================================= kWinkers =================================

const Game kWinkers = {
    .name = "winkers",
    .formal_name = "Winkers",
    .solver = &kTierSolver,
    .solver_api = &kWinkersSolverApi,
    .gameplay_api = &kWinkersGameplayApi,
    .uwapi = NULL,

    .Init = WinkersInit,
    .Finalize = WinkersFinalize,
};

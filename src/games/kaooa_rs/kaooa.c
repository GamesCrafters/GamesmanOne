/**
 * @file kaooa.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of Kaooa.
 * @version 1.0.0
 * @date 2024-11-05
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

#include "games/kaooa/kaooa.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // sprintf, sscanf
#include <stdlib.h>   // atoi
#include <string.h>   // strlen

#include "core/constants.h"
#include "core/hash/generic.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

// ============================= Type Definitions =============================

typedef union {
    struct {
        int8_t src;
        int8_t dest;
        int8_t capture;
    } unpacked;
    Move hashed;
} KaooaMove;

// ================================= Constants =================================

enum {
    kBoardSize = 10,
};

static const int8_t kNumNeighbors[kBoardSize] = {
    2, 2, 2, 2, 2, 4, 4, 4, 4, 4,
};
static const int8_t kNeighbors[kBoardSize][4] = {
    {5, 9},       {5, 6},       {6, 7},       {7, 8},       {8, 9},
    {0, 1, 6, 9}, {1, 2, 5, 7}, {2, 3, 6, 8}, {3, 4, 7, 9}, {0, 4, 5, 8},
};

/** There are excactly 2 ways to jump at each intersecion.
 * kJumps[i][j][0]: index of the intersection being jumpped over when
 * performing the j-th jump at intersection i.
 * kJumps[i][j][1]: index of the destination intersection when performing the
 * j-th jump at intersection i. */
static const int8_t kJumps[kBoardSize][2][2] = {
    {{5, 6}, {9, 8}}, {{5, 9}, {6, 7}}, {{6, 5}, {7, 8}}, {{7, 6}, {8, 9}},
    {{8, 7}, {9, 5}}, {{6, 2}, {9, 4}}, {{5, 0}, {7, 3}}, {{6, 1}, {8, 4}},
    {{7, 2}, {9, 0}}, {{5, 1}, {8, 3}},
};

static const int8_t kCaptured[kBoardSize][kBoardSize] = {
    {-1, -1, -1, -1, -1, -1, 5, -1, 9, -1},
    {-1, -1, -1, -1, -1, -1, -1, 6, -1, 5},
    {-1, -1, -1, -1, -1, 6, -1, -1, 7, -1},
    {-1, -1, -1, -1, -1, -1, 7, -1, -1, 8},
    {-1, -1, -1, -1, -1, 9, -1, 8, -1, -1},
    {-1, -1, 6, -1, 9, -1, -1, -1, -1, -1},
    {5, -1, -1, 7, -1, -1, -1, -1, -1, -1},
    {-1, 6, -1, -1, 8, -1, -1, -1, -1, -1},
    {9, -1, 7, -1, -1, -1, -1, -1, -1, -1},
    {-1, 5, -1, 8, -1, -1, -1, -1, -1, -1},
};

static const KaooaMove kKaooaMoveInit = {
    .unpacked = {.src = -1, .dest = -1, .capture = -1},
};

static const char kPieceToPlaceAtTurn[3] = {'-', 'C', 'V'};

// ============================= Variant Settings =============================

static bool mandatory_capture;

// ============================== kKaooaSolverApi ==============================

static Tier KaooaGetInitialTier(void) { return 0; }

static Position KaooaGetInitialPosition(void) {
    static ConstantReadOnlyString kInitialBoard = "----------";
    return GenericHashHashLabel(0, kInitialBoard, 1);
}

static int64_t KaooaGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static int KaooaGenerateCrowMoves(Tier tier,
                                  const char board[static kBoardSize],
                                  Move moves[static kTierSolverNumMovesMax]) {
    KaooaMove m = kKaooaMoveInit;
    int ret = 0;
    if (tier < 7) {  // Placement phase
        for (int8_t i = 0; i < kBoardSize; ++i) {
            if (board[i] == '-') {
                m.unpacked.dest = i;
                moves[ret++] = m.hashed;
            }
        }
    } else {  // Movement phase.
        assert(tier == 7);
        for (int8_t i = 0; i < kBoardSize; ++i) {
            if (board[i] != 'C') continue;
            m.unpacked.src = i;
            for (int8_t j = 0; j < kNumNeighbors[i]; ++j) {
                int8_t neighbor = kNeighbors[i][j];
                if (board[neighbor] == '-') {
                    m.unpacked.dest = neighbor;
                    moves[ret++] = m.hashed;
                }
            }
        }
    }

    return ret;
}

static int8_t FindVulture(const char board[static kBoardSize]) {
    for (int8_t i = 0; i < kBoardSize; ++i) {
        if (board[i] == 'V') return i;
    }

    return -1;
}

static int KaooaGenerateVultureMoves(
    const char board[static kBoardSize],
    Move moves[static kTierSolverNumMovesMax]) {
    //
    KaooaMove m = kKaooaMoveInit;
    int8_t vulture = FindVulture(board);
    int ret = 0;
    if (vulture < 0) {  // Placement
        for (int8_t i = 0; i < kBoardSize; ++i) {
            if (board[i] == '-') {
                m.unpacked.dest = i;
                moves[ret++] = m.hashed;
            }
        }
    } else {  // Movement
        m.unpacked.src = vulture;
        // Jump and capture.
        for (int j = 0; j < 2; ++j) {  // There are always 2 possible jumps.
            int8_t mid = kJumps[vulture][j][0];
            int8_t dest = kJumps[vulture][j][1];
            if (board[mid] == 'C' && board[dest] == '-') {
                m.unpacked.capture = mid;
                m.unpacked.dest = dest;
                moves[ret++] = m.hashed;
            }
        }

        // Moving to a neighboring slot; available only if capturing is not
        // possible or if madatory captures are disabled.
        if (!mandatory_capture || ret == 0) {
            m.unpacked.capture = -1;
            for (int8_t i = 0; i < kNumNeighbors[vulture]; ++i) {
                int8_t neighbor = kNeighbors[vulture][i];
                if (board[neighbor] == '-') {
                    m.unpacked.dest = neighbor;
                    moves[ret++] = m.hashed;
                }
            }
        }
    }

    return ret;
}

static int KaooaGenerateMoves(TierPosition tier_position,
                              Move moves[static kTierSolverNumMovesMax]) {
    // Unhash
    char board[kBoardSize];
    Tier tier = tier_position.tier;
    Position pos = tier_position.position;
    bool success = GenericHashUnhashLabel(tier, pos, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurnLabel(tier, pos);
    if (turn == 1) return KaooaGenerateCrowMoves(tier, board, moves);

    return KaooaGenerateVultureMoves(board, moves);
}

static int CrowsCount(const char board[static kBoardSize]) {
    int count = 0;
    for (int i = 0; i < kBoardSize; ++i) {
        count += (board[i] == 'C');
    }

    return count;
}

static Value KaooaPrimitive(TierPosition tier_position) {
    // The game may come to an end when:
    // 1. it is crow's turn and exactly 4 crows have been captured
    // 2. it is vulture's turn but there are no moves

    // Unhash
    char board[kBoardSize];
    Tier tier = tier_position.tier;
    Position pos = tier_position.position;
    bool success = GenericHashUnhashLabel(tier, pos, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurnLabel(tier, pos);
    int num_crows = CrowsCount(board);
    if (turn == 1 && tier - num_crows == 4) return kLose;
    if (turn == 2) {
        Move moves[kTierSolverNumMovesMax];
        int num_moves = KaooaGenerateMoves(tier_position, moves);
        if (num_moves == 0) return kLose;
    }

    return kUndecided;
}

static Tier NextTier(Tier this_tier, int turn) {
    switch (this_tier) {
        case 0:
            return 1;
        case 7:
            return 7;
        default:
            return this_tier + (turn == 1);
    }
}

static TierPosition KaooaDoMove(TierPosition tier_position, Move move) {
    // Unhash
    char board[kBoardSize];
    Tier tier = tier_position.tier;
    Position pos = tier_position.position;
    bool success = GenericHashUnhashLabel(tier, pos, board);
    assert(success);
    (void)success;
    int turn =
        GenericHashGetTurnLabel(tier_position.tier, tier_position.position);
    KaooaMove m = {.hashed = move};
    assert(m.unpacked.dest >= 0);
    if (m.unpacked.src < 0) {  // Placement
        board[m.unpacked.dest] = kPieceToPlaceAtTurn[turn];
    } else if (m.unpacked.capture < 0) {  // Movement without capturing
        board[m.unpacked.dest] = board[m.unpacked.src];
        board[m.unpacked.src] = '-';
    } else {  // Move and capture.
        assert(turn == 2);
        board[m.unpacked.dest] = 'V';
        board[m.unpacked.capture] = board[m.unpacked.src] = '-';
    }

    TierPosition child;
    child.tier = NextTier(tier, turn);
    child.position = GenericHashHashLabel(child.tier, board, 3 - turn);

    return child;
}

static bool KaooaIsLegalPosition(TierPosition tier_position) {
    // This function identifies the following easy-to-observe illegal positions:
    // it is the vulture's turn but at least 4 crows have been captured

    // Unhash
    char board[kBoardSize];
    Tier tier = tier_position.tier;
    Position pos = tier_position.position;
    bool success = GenericHashUnhashLabel(tier, pos, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurnLabel(tier, pos);
    int num_crows = CrowsCount(board);
    int8_t vulture = FindVulture(board);
    switch (tier) {
        case 0:
            return true;
        case 1:
            return (turn == 1 && vulture >= 0) || (turn == 2 && vulture < 0);
        case 2:
        case 3:
        case 4:
        case 5:
            return num_crows > 1 || turn == 1;
        case 6:
            return num_crows > 2 || turn == 1;
        case 7:
            return num_crows > 3 || turn == 1;
    }

    return false;
}

static int KaooaGetChildTiers(
    Tier tier, Tier children[static kTierSolverNumChildTiersMax]) {
    assert(tier >= 0 && tier <= 7);
    if (tier < 7) {
        children[0] = tier + 1;
        return 1;
    }

    return 0;
}

static int KaooaGetTierName(Tier tier,
                            char name[static kDbFileNameLengthMax + 1]) {
    assert(tier >= 0 && tier <= 7);
    if (tier < 7) {
        sprintf(name, "%" PRITier "_dropped", tier);
    } else {
        sprintf(name, "moving_phase");
    }

    return kNoError;
}

static const TierSolverApi kKaooaSolverApi = {
    .GetInitialTier = KaooaGetInitialTier,
    .GetInitialPosition = KaooaGetInitialPosition,
    .GetTierSize = KaooaGetTierSize,

    .GenerateMoves = KaooaGenerateMoves,
    .Primitive = KaooaPrimitive,
    .DoMove = KaooaDoMove,
    .IsLegalPosition = KaooaIsLegalPosition,

    .GetChildTiers = KaooaGetChildTiers,
    .GetTierName = KaooaGetTierName,
};

// ============================= kKaooaGameplayApi =============================

static MoveArray KaooaGenerateMovesGameplay(TierPosition tier_position) {
    Move moves[kTierSolverNumMovesMax];
    int num_moves = KaooaGenerateMoves(tier_position, moves);
    MoveArray ret;
    MoveArrayInit(&ret);
    for (int i = 0; i < num_moves; ++i) {
        MoveArrayAppend(&ret, moves[i]);
    }

    return ret;
}

static ConstantReadOnlyString kKaooaPositionStringFormat =
    "                 [1]                  |                  [%c]\n"
    "                 / \\                  |                  / \\\n"
    "                |   |                 |                 |   |\n"
    "                |   |                 |                 |   |\n"
    "               /     \\                |                /     \\\n"
    "              |       |               |               |       |\n"
    "[5]---------[10]-----[6]----------[2] | "
    "[%c]----------[%c]-----[%c]----------[%c]\n"
    "  `--.       /         \\       .--'   |   `--.       /         \\       "
    ".--'\n"
    "      \".    |           |    .\"       |       \".    |           |    "
    ".\"\n"
    "        \"-. |           | .-\"         |         \"-. |           | "
    ".-\"\n"
    "           [9]         [7]            |            [%c]         [%c]\n"
    "          /   \".     .\"   \\           |           /   \".     .\"   "
    "\\\n"
    "         /      \"[8]\"      \\          |          /      \"[%c]\"      "
    "\\\n"
    "        |      .-' '-.      |         |         |      .-' '-.      |\n"
    "        |    .\"       \".    |         |         |    .\"       \".    "
    "|\n"
    "       /  .-\"           \"-.  \\        |        /  .-\"           \"-.  "
    "\\\n"
    "      |.-\"                 \"-.|       |       |.-\"                 "
    "\"-.|\n"
    "     [4]                     [3]      |      [%c]                     "
    "[%c]\n";

static int KaooaTierPositionToString(TierPosition tier_position, char *buffer) {
    // Unhash
    char board[kBoardSize];
    Tier tier = tier_position.tier;
    Position pos = tier_position.position;
    bool success = GenericHashUnhashLabel(tier, pos, board);
    if (!success) return kGenericHashError;

    sprintf(buffer, kKaooaPositionStringFormat, board[0], board[4], board[9],
            board[5], board[1], board[8], board[6], board[7], board[3],
            board[2]);

    return kNoError;
}

static int KaooaMoveToString(Move move, char *buffer) {
    KaooaMove m = {.hashed = move};
    if (m.unpacked.src < 0) {  // Placement
        sprintf(buffer, "%d", m.unpacked.dest + 1);
    } else {
        sprintf(buffer, "%d %d", m.unpacked.src + 1, m.unpacked.dest + 1);
    }

    return kNoError;
}

static bool KaooaIsValidMoveString(ReadOnlyString move_string) {
    int length = strlen(move_string);
    return length >= 1 && length <= 4;
}

static Move KaooaStringToMove(ReadOnlyString move_string) {
    static ConstantReadOnlyString kDelim = " ";
    char move_string_copy[5];
    strcpy(move_string_copy, move_string);
    char *first = strtok(move_string_copy, kDelim);
    char *second = strtok(NULL, kDelim);
    KaooaMove m = kKaooaMoveInit;
    if (second == NULL) {
        m.unpacked.dest = atoi(first) - 1;
    } else {
        m.unpacked.src = atoi(first) - 1;
        m.unpacked.dest = atoi(second) - 1;
        m.unpacked.capture = kCaptured[m.unpacked.src][m.unpacked.dest];
    }

    return m.hashed;
}

static const GameplayApiCommon KaooaGameplayApiCommon = {
    .GetInitialPosition = KaooaGetInitialPosition,
    .position_string_length_max = 1300,

    .move_string_length_max = 4,
    .MoveToString = KaooaMoveToString,

    .IsValidMoveString = KaooaIsValidMoveString,
    .StringToMove = KaooaStringToMove,
};

static const GameplayApiTier KaooaGameplayApiTier = {
    .GetInitialTier = KaooaGetInitialTier,

    .TierPositionToString = KaooaTierPositionToString,

    .GenerateMoves = KaooaGenerateMovesGameplay,
    .DoMove = KaooaDoMove,
    .Primitive = KaooaPrimitive,
};

static const GameplayApi kKaooaGameplayApi = {
    .common = &KaooaGameplayApiCommon,
    .tier = &KaooaGameplayApiTier,
};

// ================================= KaooaInit =================================

static int KaooaInit(void *aux) {
    static const int pieces_init[8][10] = {
        {'-', 10, 10, 'C', 0, 0, 'V', 0, 0, -1},
        {'-', 8, 9, 'C', 1, 1, 'V', 0, 1, -1},
        {'-', 7, 8, 'C', 1, 2, 'V', 1, 1, -1},
        {'-', 6, 8, 'C', 1, 3, 'V', 1, 1, -1},
        {'-', 5, 8, 'C', 1, 4, 'V', 1, 1, -1},
        {'-', 4, 8, 'C', 1, 5, 'V', 1, 1, -1},
        {'-', 3, 7, 'C', 2, 6, 'V', 1, 1, -1},
        {'-', 2, 6, 'C', 3, 7, 'V', 1, 1, -1},
    };
    (void)aux;
    GenericHashReinitialize();
    bool success =
        GenericHashAddContext(1, kBoardSize, pieces_init[0], NULL, 0);
    for (Tier tier = 1; tier <= 7; ++tier) {
        success &=
            GenericHashAddContext(0, kBoardSize, pieces_init[tier], NULL, tier);
    }
    if (!success) return kGenericHashError;

    return kNoError;
}

// =============================== KaooaFinalize ===============================

static int KaooaFinalize(void) { return kNoError; }

// ================================== kKaooa ==================================

const Game kKaooa = {
    .name = "kaooa",
    .formal_name = "Kaooa",
    .solver = &kTierSolver,
    .solver_api = &kKaooaSolverApi,
    .gameplay_api = &kKaooaGameplayApi,

    .Init = KaooaInit,
    .Finalize = KaooaFinalize,
};

/**
 * @file gobblet_gobblers.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of Gobblet Gobblers.
 * @details See https://docs.racket-lang.org/games/gobblet.html for game rules.
 * @version 1.0.0
 * @date 2024-12-07
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

#include "games/gobblet/gobblet_gobblers.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t, int8_t
#include <stdio.h>    //
#include <stdlib.h>   // atoi
#include <string.h>   // memset

#include "core/constants.h"
#include "core/generic_hash/generic_hash.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

// =================================== Types ===================================

typedef union {
    int8_t count[2]; /**< [0, 2] each. */
    int16_t hash;
} RemainingPieceConfig;

typedef union {
    /**
     * @brief Remaining piece configurations of each size.
     * \c configs[i].count[0] equals the number of remaining pieces of
     * player X with size i. \c configs[i].count[1] equals the number of
     * remaining pieces of player O with size i. Define size 0 small, size 1
     * medium, size 2 large.
     */
    RemainingPieceConfig configs[3];
    Tier hash; /**< Hashed tier value. */
} GobbletGobblersTier;

typedef struct {
    char board[3][9];  // 0 for small board, 1 for medium, and 2 for large.
    int turn;
} GobbletGobblersPosition;

typedef union {
    struct {
        int8_t add_size; /**< size of piece to add */
        int8_t src;      /**< src slot on board; [0, 8] */
        int8_t dest;     /**< dest slot on board; [0, 8] */
    } unpacked;
    Move hash;
} GobbletGobblersMove;

// ================================= Constants =================================

static const GobbletGobblersTier kGobbletGobblersTierInit = {.hash = 0};

static const GobbletGobblersMove kGobbletGobblersMoveInit = {
    .unpacked = {.add_size = -1, .src = -1, .dest = -1},
};

enum GobbletGobblersPieceIndex { X = 0, O = 1, Blank = 2 };

static const int kSymmetryMatrix[8][9] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8}, {2, 5, 8, 1, 4, 7, 0, 3, 6},
    {8, 7, 6, 5, 4, 3, 2, 1, 0}, {6, 3, 0, 7, 4, 1, 8, 5, 2},
    {2, 1, 0, 5, 4, 3, 8, 7, 6}, {0, 3, 6, 1, 4, 7, 2, 5, 8},
    {6, 7, 8, 3, 4, 5, 0, 1, 2}, {8, 5, 2, 7, 4, 1, 6, 3, 0},
};

// ============================= Variant Settings =============================

// ========================= kGobbletGobblersSolverApi =========================

static Tier GobbletGobblersGetInitialTier(void) {
    GobbletGobblersTier t;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 2; ++j) {
            t.configs[i].count[j] = 2;
        }
    }

    return t.hash;
}

static Position Hash(GobbletGobblersTier t, const GobbletGobblersPosition *p) {
    // Separately hash each sub-board and combine the sub-hashes.
    Position ret = 0;
    for (int i = 0; i < 3; ++i) {
        int64_t label = t.configs[i].hash;
        int64_t base = GenericHashNumPositionsLabel(label);

        // Note that we are not using the "turn" feature of generic hash here.
        int64_t offset =
            GenericHashHashLabel(label, (const char *)p->board[i], 1);
        ret = ret * base + offset;
    }

    return (ret << 1) + p->turn - 1;
}

static bool Unhash(TierPosition tp, GobbletGobblersTier *t,
                   GobbletGobblersPosition *p) {
    t->hash = tp.tier;
    Position pos = tp.position;
    p->turn = 1 + (pos & 1);
    pos >>= 1;
    for (int i = 2; i >= 0; --i) {
        int64_t label = t->configs[i].hash;
        int64_t base = GenericHashNumPositionsLabel(label);
        int64_t offset = pos % base;
        pos /= base;
        bool success =
            GenericHashUnhashLabel(label, offset, (char *)p->board[i]);
        if (!success) return false;
    }

    return true;
}

static Position GobbletGobblersGetInitialPosition(void) {
    GobbletGobblersTier t = {.hash = GobbletGobblersGetInitialTier()};
    GobbletGobblersPosition p;
    memset(p.board, '-', sizeof(p.board));
    p.turn = 1;

    return Hash(t, &p);
}

static int64_t GobbletGobblersGetTierSize(Tier tier) {
    GobbletGobblersTier t = {.hash = tier};
    int64_t ret = 1;
    for (int i = 0; i < 3; ++i) {
        int64_t label = t.configs[i].hash;
        ret *= GenericHashNumPositionsLabel(label);
    }

    return ret << 1;  // Add the turn bit.
}

static void GetHeights(int8_t heights[static 9],
                       const GobbletGobblersPosition *p) {
    memset(heights, -1, 9);
    for (int size = 2; size >= 0; --size) {
        for (int i = 0; i < 9; ++i) {
            if (p->board[size][i] != '-') {
                assert(p->board[size][i] == 'X' || p->board[size][i] == 'O');
                heights[i] = size;
            }
        }
    }
}

static void GetFaces(char faces[static 9], const GobbletGobblersPosition *p) {
    memset(faces, '-', 9);
    for (int size = 2; size >= 0; --size) {
        for (int i = 0; i < 9; ++i) {
            if (p->board[size][i] != '-') {
                assert(p->board[size][i] == 'X' || p->board[size][i] == 'O');
                faces[i] = p->board[size][i];
            }
        }
    }
}

static const char kTurnToPiece[] = {'-', 'X', 'O'};

static void GenerateMovesAddPiece(MoveArray *moves, int turn,
                                  GobbletGobblersTier t,
                                  const int8_t heights[static 9]) {
    GobbletGobblersMove m = kGobbletGobblersMoveInit;
    for (int8_t size = 0; size <= 2; ++size) {
        if (t.configs[size].count[turn - 1] == 0) continue;
        assert(t.configs[size].count[turn - 1] > 0);
        for (m.unpacked.dest = 0; m.unpacked.dest < 9; ++m.unpacked.dest) {
            if (heights[m.unpacked.dest] >= size) continue;
            m.unpacked.add_size = size;
            MoveArrayAppend(&moves, m.hash);
        }
    }
}

static void GenerateMovesMovePiece(MoveArray *moves, int turn,
                                   const int8_t heights[static 9],
                                   const char faces[static 9]) {
    char piece = kTurnToPiece[turn];
    GobbletGobblersMove m = kGobbletGobblersMoveInit;
    for (m.unpacked.src = 0; m.unpacked.src < 9; ++m.unpacked.src) {
        if (faces[m.unpacked.src] != piece) continue;
        for (m.unpacked.dest = 0; m.unpacked.dest < 9; ++m.unpacked.dest) {
            if (heights[m.unpacked.dest] >= heights[m.unpacked.src]) continue;
            MoveArrayAppend(&moves, m.hash);
        }
    }
}

static MoveArray GenerateMovesInternal(GobbletGobblersTier t,
                                       const GobbletGobblersPosition *p) {
    int8_t heights[9];
    char faces[9];
    GetHeights(heights, &p);
    GetFaces(faces, &p);

    MoveArray ret;
    MoveArrayInit(&ret);
    GenerateMovesAddPiece(&ret, p->turn, t, heights);
    GenerateMovesMovePiece(&ret, p->turn, heights, faces);

    return ret;
}

static MoveArray GobbletGobblersGenerateMoves(TierPosition tier_position) {
    // Unhash
    GobbletGobblersTier t;
    GobbletGobblersPosition p;
    Unhash(tier_position, &t, &p);

    return GenerateMovesInternal(t, &p);
}

static bool HasThreeInARow(const char faces[static 9], char piece) {
    static const int kRows[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6},
        {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6},
    };
    for (int i = 0; i < 8; ++i) {
        if (faces[kRows[i][0]] == faces[kRows[i][1]] &&
            faces[kRows[i][1]] == faces[kRows[i][2]] &&
            faces[kRows[i][2]] == piece) {
            return true;
        }
    }

    return false;
}

static Value GobbletGobblersPrimitive(TierPosition tier_position) {
    // Unhash
    GobbletGobblersTier t;
    GobbletGobblersPosition p;
    Unhash(tier_position, &t, &p);
    char faces[9];
    GetFaces(faces, &p);
    char my_piece = kTurnToPiece[p.turn];
    char opponent_piece = kTurnToPiece[3 - p.turn];

    if (HasThreeInARow(faces, my_piece)) {
        // The current player wins if there is a 3-in-a-row of the current
        // player's pieces, regardless of whether there is a 3-in-a-row of the
        // opponent's pieces.
        return kWin;
    } else if (HasThreeInARow(faces, opponent_piece)) {
        // If the current player is not winning but there's a 3-in-a-row of the
        // opponent's pieces, then the current player loses.
        return kLose;
    }

    // Neither side is winning.
    return kUndecided;
}

static TierPosition DoMoveAddPiece(GobbletGobblersTier t,
                                   GobbletGobblersPosition *p,
                                   GobbletGobblersMove m) {
    // Adjust remaining pieces
    int8_t size = m.unpacked.add_size;
    --t.configs[size].count[p->turn - 1];
    assert(t.configs[size].count[p->turn - 1] >= 0);

    // Adjust board
    char piece = kTurnToPiece[p->turn];
    assert(p->board[size][m.unpacked.dest] == '-');
    assert(size == 2 || (size == 1 && p->board[2][m.unpacked.dest] == '-') ||
           (size == 0 && p->board[1][m.unpacked.dest] == '-' &&
            p->board[2][m.unpacked.dest] == '-'));
    p->board[size][m.unpacked.dest] = piece;

    // Hash
    TierPosition ret = {.tier = t.hash, .position = Hash(t, p)};

    return ret;
}

static TierPosition DoMoveMovePiece(GobbletGobblersTier t,
                                    GobbletGobblersPosition *p,
                                    GobbletGobblersMove m) {
    // Locate exposed piece and determine its size
    int8_t src = m.unpacked.src;
    int8_t size = (p->board[2][src] != '-')   ? 2
                  : (p->board[1][src] != '-') ? 1
                                              : 0;
    assert(p->board[size][src] == kTurnToPiece[p->turn]);

    // Move piece to dest
    int8_t dest = m.unpacked.dest;
    assert(p->board[size][dest] == '-');
    p->board[size][dest] = p->board[size][src];
    p->board[size][src] = '-';

    // Hash
    TierPosition ret = {.tier = t.hash, .position = Hash(t, p)};

    return ret;
}

static TierPosition DoMoveInternal(GobbletGobblersTier t,
                                   GobbletGobblersPosition p,
                                   GobbletGobblersMove m) {
    if (m.unpacked.add_size < 0) return DoMoveAddPiece(t, &p, m);

    return DoMoveMovePiece(t, &p, m);
}

static TierPosition GobbletGobblersDoMove(TierPosition tier_position,
                                          Move move) {
    // Unhash
    GobbletGobblersTier t;
    GobbletGobblersPosition p;
    Unhash(tier_position, &t, &p);
    GobbletGobblersMove m = {.hash = move};

    return DoMoveInternal(t, p, m);
}

static bool GobbletGobblersIsLegalPosition(TierPosition tier_position) {
    return true;
}

static Position ApplySymmetry(GobbletGobblersTier t,
                              const GobbletGobblersPosition *p,
                              int symmetry_index) {
    GobbletGobblersPosition symm = {.turn = p->turn};
    for (int size = 0; size <= 2; ++size) {
        for (int i = 0; i < 9; ++i) {
            symm.board[size][i] =
                p->board[size][kSymmetryMatrix[symmetry_index][i]];
        }
    }

    return Hash(t, &symm);
}

static Position GobbletGobblersGetCanonicalPosition(
    TierPosition tier_position) {
    // Unhash
    GobbletGobblersTier t;
    GobbletGobblersPosition p;
    Unhash(tier_position, &t, &p);

    Position ret = tier_position.position;
    for (int i = 1; i < 8; ++i) {
        Position symm = ApplySymmetry(t, &p, i);
        if (symm < ret) ret = symm;
    }

    return ret;
}

static TierPositionArray GobbletGobblersGetCanonicalChildPositions(
    TierPosition tier_position) {
    // Unhash
    GobbletGobblersTier t;
    GobbletGobblersPosition p;
    Unhash(tier_position, &t, &p);

    // Generate moves
    MoveArray moves = GenerateMovesInternal(t, &p);

    // Do moves
    TierPositionArray ret;
    TierPositionArrayInit(&ret);
    TierPositionHashSet dedup;
    TierPositionHashSetInit(&dedup, 0.5);
    for (int64_t i = 0; i < moves.size; ++i) {
        GobbletGobblersMove m = {.hash = moves.array[i]};
        TierPosition child = DoMoveInternal(t, p, m);
        child.position = GobbletGobblersGetCanonicalPosition(child);
        if (!TierPositionHashSetContains(&dedup, child)) {
            TierPositionHashSetAdd(&dedup, child);
            TierPositionArrayAppend(&ret, child);
        }
    }
    TierPositionHashSetDestroy(&dedup);
    MoveArrayDestroy(&moves);

    return ret;
}

static TierArray GobbletGobblersGetChildTiers(Tier tier) {
    TierArray ret;
    TierArrayInit(&ret);
    GobbletGobblersTier t = {.hash = tier};
    for (int size = 0; size <= 2; ++size) {
        for (int player = 0; player <= 1; ++player) {
            if (t.configs[size].count[player]) {
                assert(t.configs[size].count[player] > 0);
                --t.configs[size].count[player];
                TierArrayAppend(&ret, t.hash);
                ++t.configs[size].count[player];
            }
        }
    }

    return ret;
}

static int GobbletGobblersGetTierName(char *name, Tier tier) {
    GobbletGobblersTier t = {.hash = tier};
    sprintf(name, "%d%d%d%d%d%d", t.configs[0].count[0], t.configs[0].count[1],
            t.configs[1].count[0], t.configs[1].count[1], t.configs[2].count[0],
            t.configs[2].count[1]);

    return kNoError;
}

static const TierSolverApi kGobbletGobblersSolverApi = {
    .GetInitialTier = GobbletGobblersGetInitialTier,
    .GetInitialPosition = GobbletGobblersGetInitialPosition,
    .GetTierSize = GobbletGobblersGetTierSize,

    .GenerateMoves = GobbletGobblersGenerateMoves,
    .Primitive = GobbletGobblersPrimitive,
    .DoMove = GobbletGobblersDoMove,
    .IsLegalPosition = GobbletGobblersIsLegalPosition,
    .GetCanonicalPosition = GobbletGobblersGetCanonicalPosition,
    .GetCanonicalChildPositions = GobbletGobblersGetCanonicalChildPositions,

    .GetChildTiers = GobbletGobblersGetChildTiers,
    .GetTierName = GobbletGobblersGetTierName,
};

// ======================== kGobbletGobblersGameplayApi ========================

static int GobbletGobblersTierPositionToString(TierPosition tier_position,
                                               char *buffer) {}

static const GameplayApiTier GobbletGobblersGameplayApiTier = {
    .GetInitialTier = GobbletGobblersGetInitialTier,

    .TierPositionToString = GobbletGobblersTierPositionToString,

    .GenerateMoves = GobbletGobblersGenerateMoves,
    .DoMove = GobbletGobblersDoMove,
    .Primitive = GobbletGobblersPrimitive,
};

static const GameplayApi kGobbletGobblersGameplayApi = {
    .common = NULL,
    .tier = &GobbletGobblersGameplayApiTier,
};

// ===================== GobbletGobblersGetCurrentVariant =====================

// ====================== GobbletGobblersSetVariantOption ======================

// ============================ GobbletGobblersInit ============================

static int GobbletGobblersInit(void *aux) {
    (void)aux;  // Unused.
    GenericHashReinitialize();
    int pieces_init[] = {'X', 0, 2, 'O', 0, 2, '-', 0, 2, -1};  // Placeholder.
    RemainingPieceConfig rpc;
    for (rpc.count[X] = 0; rpc.count[X] <= 2; ++rpc.count[X]) {
        for (rpc.count[O] = 0; rpc.count[O] <= 2; ++rpc.count[O]) {
            pieces_init[X * 3 + 1] = pieces_init[X * 3 + 2] = rpc.count[X];
            pieces_init[O * 3 + 1] = pieces_init[O * 3 + 2] = rpc.count[O];
            pieces_init[Blank * 3 + 1] = pieces_init[Blank * 3 + 2] =
                9 - rpc.count[X] - rpc.count[O];
            bool success =
                GenericHashAddContext(1, 9, pieces_init, NULL, rpc.hash);
            if (!success) return kGenericHashError;
        }
    }

    return kNoError;
}

// ========================== GobbletGobblersFinalize ==========================

static int GobbletGobblersFinalize(void) { return kNoError; }

// =========================== kGobbletGobblersUwapi ===========================

// ============================= kGobbletGobblers =============================

const Game kGobbletGobblers = {
    .name = "gobbletg",
    .formal_name = "Gobblet Gobblers",
    .solver = &kTierSolver,
    .solver_api = &kGobbletGobblersSolverApi,
    .gameplay_api = &kGobbletGobblersGameplayApi,
    .uwapi = NULL,

    .Init = GobbletGobblersInit,
    .Finalize = GobbletGobblersFinalize,

    .GetCurrentVariant = NULL,
    .SetVariantOption = NULL,
};

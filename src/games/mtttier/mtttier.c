/**
 * @file mtttier.c
 * @author Dan Garcia: designed and developed of the original version (mttt.c in
 * GamesmanClassic.)
 * @author Max Delgadillo: developed of the first tiered version using generic
 * hash.
 * @author Robert Shi (robertyishi@berkeley.edu): adapted to the new system.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of Tic-Tac-Tier.
 * @version 1.0.7
 * @date 2024-09-07
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

#include "games/mtttier/mtttier.h"

#include <assert.h>    // assert
#include <ctype.h>     // toupper
#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t
#include <stdio.h>     // fprintf, stderr, sprintf
#include <stdlib.h>    // atoi

#include "core/generic_hash/generic_hash.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

// Game, Solver, Gameplay, and UWAPI Functions

static int MtttierInit(void *aux);
static int MtttierFinalize(void);

static const GameVariant *MtttierGetCurrentVariant(void);
static int MtttierSetVariantOption(int option, int selection);

static Tier MtttierGetInitialTier(void);
static Position MtttierGetInitialPosition(void);

static int64_t MtttierGetTierSize(Tier tier);
static MoveArray MtttierGenerateMoves(TierPosition tier_position);
static Value MtttierPrimitive(TierPosition tier_position);
static TierPosition MtttierDoMove(TierPosition tier_position, Move move);
static bool MtttierIsLegalPosition(TierPosition tier_position);
static Position MtttierGetCanonicalPosition(TierPosition tier_position);
static PositionArray MtttierGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier);
static TierArray MtttierGetChildTiers(Tier tier);
static TierType MtttierGetTierType(Tier tier);
static int MtttierGetTierName(Tier tier,
                              char name[static kDbFileNameLengthMax + 1]);

static int MtttTierPositionToString(TierPosition tier_position, char *buffer);
static int MtttierMoveToString(Move move, char *buffer);
static bool MtttierIsValidMoveString(ReadOnlyString move_string);
static Move MtttierStringToMove(ReadOnlyString move_string);

static bool MtttierIsLegalFormalPosition(ReadOnlyString formal_position);
static TierPosition MtttierFormalPositionToTierPosition(
    ReadOnlyString formal_position);
static CString MtttierTierPositionToFormalPosition(TierPosition tier_position);
static CString MtttierTierPositionToAutoGuiPosition(TierPosition tier_position);
static CString MtttierMoveToFormalMove(TierPosition tier_position, Move move);
static CString MtttierMoveToAutoGuiMove(TierPosition tier_position, Move move);

// Solver API Setup
static const TierSolverApi kSolverApi = {
    .GetInitialTier = &MtttierGetInitialTier,
    .GetInitialPosition = &MtttierGetInitialPosition,

    .GetTierSize = &MtttierGetTierSize,
    .GenerateMoves = &MtttierGenerateMoves,
    .Primitive = &MtttierPrimitive,
    .DoMove = &MtttierDoMove,
    .IsLegalPosition = &MtttierIsLegalPosition,
    .GetCanonicalPosition = &MtttierGetCanonicalPosition,
    .GetCanonicalChildPositions = NULL,
    .GetCanonicalParentPositions = &MtttierGetCanonicalParentPositions,
    .GetPositionInSymmetricTier = NULL,
    .GetChildTiers = &MtttierGetChildTiers,
    .GetTierType = &MtttierGetTierType,
    .GetCanonicalTier = NULL,

    .GetTierName = &MtttierGetTierName,
};

// Gameplay API Setup

static const GameplayApiCommon kMtttierGameplayApiCommon = {
    .GetInitialPosition = &MtttierGetInitialPosition,
    .position_string_length_max = 120,

    .move_string_length_max = 1,
    .MoveToString = &MtttierMoveToString,

    .IsValidMoveString = &MtttierIsValidMoveString,
    .StringToMove = &MtttierStringToMove,
};

static const GameplayApiTier kMtttierGameplayApiTier = {
    .GetInitialTier = &MtttierGetInitialTier,

    .TierPositionToString = &MtttTierPositionToString,

    .GenerateMoves = &MtttierGenerateMoves,
    .DoMove = &MtttierDoMove,
    .Primitive = &MtttierPrimitive,
};

static const GameplayApi kMtttierGameplayApi = {
    .common = &kMtttierGameplayApiCommon,
    .tier = &kMtttierGameplayApiTier,
};

// UWAPI Setup

static const UwapiTier kMtttierUwapiTier = {
    .GenerateMoves = &MtttierGenerateMoves,
    .DoMove = &MtttierDoMove,
    .Primitive = &MtttierPrimitive,
    .IsLegalFormalPosition = &MtttierIsLegalFormalPosition,
    .FormalPositionToTierPosition = &MtttierFormalPositionToTierPosition,
    .TierPositionToFormalPosition = &MtttierTierPositionToFormalPosition,
    .TierPositionToAutoGuiPosition = &MtttierTierPositionToAutoGuiPosition,
    .MoveToFormalMove = &MtttierMoveToFormalMove,
    .MoveToAutoGuiMove = &MtttierMoveToAutoGuiMove,
    .GetInitialTier = &MtttierGetInitialTier,
    .GetInitialPosition = &MtttierGetInitialPosition,
    .GetRandomLegalTierPosition = NULL,
};

static const Uwapi kMtttierUwapi = {.tier = &kMtttierUwapiTier};

const Game kMtttier = {
    .name = "mtttier",
    .formal_name = "Tic-Tac-Tier",
    .solver = &kTierSolver,
    .solver_api = (const void *)&kSolverApi,
    .gameplay_api = (const GameplayApi *)&kMtttierGameplayApi,
    .uwapi = &kMtttierUwapi,

    .Init = &MtttierInit,
    .Finalize = &MtttierFinalize,

    .GetCurrentVariant = &MtttierGetCurrentVariant,
    .SetVariantOption = &MtttierSetVariantOption,
};

// Helper Types and Global Variables

static const int kNumRowsToCheck = 8;
static const int kRowsToCheck[8][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6},
    {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6},
};
static const int kNumSymmetries = 8;
static const int kSymmetryMatrix[8][9] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8}, {2, 5, 8, 1, 4, 7, 0, 3, 6},
    {8, 7, 6, 5, 4, 3, 2, 1, 0}, {6, 3, 0, 7, 4, 1, 8, 5, 2},
    {2, 1, 0, 5, 4, 3, 8, 7, 6}, {0, 3, 6, 1, 4, 7, 2, 5, 8},
    {6, 7, 8, 3, 4, 5, 0, 1, 2}, {8, 5, 2, 7, 4, 1, 6, 3, 0},
};

// Helper Functions

static bool InitGenericHash(void);
static char ThreeInARow(ReadOnlyString board, const int *indices);
static bool AllFilledIn(ReadOnlyString board);
static void CountPieces(ReadOnlyString board, int *xcount, int *ocount);
static char WhoseTurn(ReadOnlyString board);
static Position DoSymmetry(TierPosition tier_position, int symmetry);
static char ConvertBlankToken(char piece);

static int MtttierInit(void *aux) {
    (void)aux;  // Unused.
    return !InitGenericHash();
}

static int MtttierFinalize(void) { return kNoError; }

static const GameVariant *MtttierGetCurrentVariant(void) {
    return NULL;  // No other variants implemented.
}

static int MtttierSetVariantOption(int option, int selection) {
    (void)option;
    (void)selection;
    return kNotImplementedError;  // No other variants implemented.
}

static Tier MtttierGetInitialTier(void) { return 0; }

// Assumes Generic Hash has been initialized.
static Position MtttierGetInitialPosition(void) {
    char board[9];
    for (int i = 0; i < 9; ++i) {
        board[i] = '-';
    }
    return GenericHashHashLabel(0, board, 1);
}

static int64_t MtttierGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static MoveArray MtttierGenerateMoves(TierPosition tier_position) {
    MoveArray moves;
    MoveArrayInit(&moves);
    char board[9] = {0};
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);
    for (Move i = 0; i < 9; ++i) {
        if (board[i] == '-') {
            MoveArrayAppend(&moves, i);
        }
    }
    return moves;
}

static Value MtttierPrimitive(TierPosition tier_position) {
    char board[9] = {0};
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);

    for (int i = 0; i < kNumRowsToCheck; ++i) {
        if (ThreeInARow(board, kRowsToCheck[i]) > 0) return kLose;
    }
    if (AllFilledIn(board)) return kTie;
    return kUndecided;
}

static TierPosition MtttierDoMove(TierPosition tier_position, Move move) {
    char board[9] = {0};
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);
    char turn = WhoseTurn(board);
    board[move] = turn;
    TierPosition ret;
    ret.tier = tier_position.tier + 1;
    ret.position = GenericHashHashLabel(ret.tier, board, 1);
    return ret;
}

static bool MtttierIsLegalPosition(TierPosition tier_position) {
    // A position is legal if and only if:
    // 1. xcount == ocount or xcount = ocount + 1 if no one is winning and
    // 2. xcount == ocount if O is winning and
    // 3. xcount == ocount + 1 if X is winning and
    // 4. only one player can be winning
    char board[9] = {0};
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);

    int xcount, ocount;
    CountPieces(board, &xcount, &ocount);
    if (xcount != ocount && xcount != ocount + 1) return false;

    bool xwin = false, owin = false;
    for (int i = 0; i < kNumRowsToCheck; ++i) {
        char row_value = ThreeInARow(board, kRowsToCheck[i]);
        xwin |= (row_value == 'X');
        owin |= (row_value == 'O');
    }
    if (xwin && owin) return false;
    if (xwin && xcount != ocount + 1) return false;
    if (owin && xcount != ocount) return false;
    return true;
}

static Position MtttierGetCanonicalPosition(TierPosition tier_position) {
    Position canonical_position = tier_position.position;
    Position new_position;

    for (int i = 0; i < kNumSymmetries; ++i) {
        new_position = DoSymmetry(tier_position, i);
        if (new_position < canonical_position) {
            // By GAMESMAN convention, the canonical position is the one with
            // the smallest hash value.
            canonical_position = new_position;
        }
    }

    return canonical_position;
}

static PositionArray MtttierGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier) {
    //
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    PositionArray parents;
    PositionArrayInit(&parents);
    if (parent_tier != tier - 1) return parents;

    PositionHashSet deduplication_set;
    PositionHashSetInit(&deduplication_set, 0.5);

    char board[9] = {0};
    GenericHashUnhashLabel(tier, position, board);

    char prev_turn = WhoseTurn(board) == 'X' ? 'O' : 'X';
    for (int i = 0; i < 9; ++i) {
        if (board[i] == prev_turn) {
            // Take piece off the board.
            board[i] = '-';
            TierPosition parent = {
                .tier = tier - 1,
                .position = GenericHashHashLabel(tier - 1, board, 1),
            };
            // Add piece back to the board.
            board[i] = prev_turn;
            if (!MtttierIsLegalPosition(parent)) {
                continue;  // Illegal.
            }
            parent.position = MtttierGetCanonicalPosition(parent);
            if (PositionHashSetContains(&deduplication_set, parent.position)) {
                continue;  // Already included.
            }
            PositionHashSetAdd(&deduplication_set, parent.position);
            PositionArrayAppend(&parents, parent.position);
        }
    }
    PositionHashSetDestroy(&deduplication_set);

    return parents;
}

static TierArray MtttierGetChildTiers(Tier tier) {
    TierArray children;
    TierArrayInit(&children);
    if (tier < 9) TierArrayAppend(&children, tier + 1);
    return children;
}

static TierType MtttierGetTierType(Tier tier) {
    (void)tier;  // No tier loops back to itself.
    return kTierTypeImmediateTransition;
}

static int MtttierGetTierName(Tier tier,
                              char name[static kDbFileNameLengthMax + 1]) {
    return sprintf(name, "%" PRITier "p", tier);
}

static int MtttTierPositionToString(TierPosition tier_position, char *buffer) {
    char board[9] = {0};
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    if (!success) return kRuntimeError;

    for (int i = 0; i < 9; ++i) {
        board[i] = ConvertBlankToken(board[i]);
    }

    static ConstantReadOnlyString kFormat =
        "         ( 1 2 3 )           : %c %c %c\n"
        "LEGEND:  ( 4 5 6 )  TOTAL:   : %c %c %c\n"
        "         ( 7 8 9 )           : %c %c %c";
    int actual_length = snprintf(
        buffer, kMtttierGameplayApiCommon.position_string_length_max + 1,
        kFormat, board[0], board[1], board[2], board[3], board[4], board[5],
        board[6], board[7], board[8]);
    if (actual_length >=
        kMtttierGameplayApiCommon.position_string_length_max + 1) {
        fprintf(
            stderr,
            "MtttierTierPositionToString: (BUG) not enough space was allocated "
            "to buffer. Please increase position_string_length_max.\n");
        return kMemoryOverflowError;
    }

    return kNoError;
}

static int MtttierMoveToString(Move move, char *buffer) {
    int actual_length =
        snprintf(buffer, kMtttierGameplayApiCommon.move_string_length_max + 1,
                 "%" PRId64, move + 1);
    if (actual_length >= kMtttierGameplayApiCommon.move_string_length_max + 1) {
        fprintf(stderr,
                "MtttierMoveToString: (BUG) not enough space was allocated "
                "to buffer. Please increase move_string_length_max.\n");
        return kMemoryOverflowError;
    }

    return kNoError;
}

static bool MtttierIsValidMoveString(ReadOnlyString move_string) {
    // Only "1" - "9" are valid move strings.
    if (move_string[0] < '1') return false;
    if (move_string[0] > '9') return false;
    if (move_string[1] != '\0') return false;

    return true;
}

static Move MtttierStringToMove(ReadOnlyString move_string) {
    assert(MtttierIsValidMoveString(move_string));
    return (Move)atoi(move_string) - 1;
}

static bool MtttierIsLegalFormalPosition(ReadOnlyString formal_position) {
    if (formal_position == NULL) return false;
    for (int i = 0; i < 9; ++i) {
        char curr = formal_position[i];
        if (curr != '-' && curr != 'o' && curr != 'x') return false;
    }
    if (formal_position[9] != '\0') return false;

    return true;
}

static TierPosition MtttierFormalPositionToTierPosition(
    ReadOnlyString formal_position) {
    // Formal position string format: 9 characters '-', 'o', or 'x'.
    char board[9];
    int piece_count = 0;
    for (int i = 0; i < 9; ++i) {
        board[i] = toupper(formal_position[i]);
        piece_count += board[i] != '-';
    }

    TierPosition ret = {
        .tier = piece_count,
        .position = GenericHashHashLabel(piece_count, board, 1),
    };

    return ret;
}

static CString MtttierTierPositionToFormalPosition(TierPosition tier_position) {
    char board[9];
    CString ret = {0};
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    if (!success) return ret;

    for (int i = 0; i < 9; ++i) {
        board[i] = tolower(board[i]);
    }
    CStringInitCopyCharArray(&ret, board);

    return ret;
}

static CString MtttierTierPositionToAutoGuiPosition(
    TierPosition tier_position) {
    //
    char board[9];
    CString ret = {0};
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    if (!success) return ret;

    char turn = WhoseTurn(board) == 'X' ? '1' : '2';
    if (!CStringInitCopyCharArray(&ret, "1_---------")) return ret;

    ret.str[0] = turn;
    for (int i = 0; i < 9; ++i) {
        ret.str[i + 2] = tolower(board[i]);
    }

    return ret;
}

static CString MtttierMoveToFormalMove(TierPosition tier_position, Move move) {
    (void)tier_position;  // Unused.
    CString ret;
    if (!CStringInitCopyCharArray(&ret, "0")) return ret;

    assert(move >= 0 && move < 9);
    ret.str[0] = (char)('0' + move);

    return ret;
}

static CString MtttierMoveToAutoGuiMove(TierPosition tier_position, Move move) {
    CString ret = {0};
    char board[9] = {0};
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    if (!success) return ret;
    if (!CStringInitCopyCharArray(&ret, "A_x_0")) return ret;

    char turn = WhoseTurn(board);
    ret.str[2] = turn == 'X' ? 'x' : 'o';
    assert(move >= 0 && move < 9);
    ret.str[4] = (char)('0' + move);

    return ret;
}

// Helper functions implementation

static bool InitGenericHash(void) {
    GenericHashReinitialize();
    int player = 1;  // No turn bit needed as we can infer the turn from board.
    int board_size = 9;
    int pieces_init_array[10] = {'-', 9, 9, 'O', 0, 0, 'X', 0, 0, -1};
    for (Tier tier = 0; tier <= 9; ++tier) {
        // Adjust piece_init_array
        pieces_init_array[1] = pieces_init_array[2] = 9 - (int)tier;
        pieces_init_array[4] = pieces_init_array[5] = (int)tier / 2;
        pieces_init_array[7] = pieces_init_array[8] = ((int)tier + 1) / 2;
        bool success = GenericHashAddContext(player, board_size,
                                             pieces_init_array, NULL, tier);
        if (!success) {
            fprintf(stderr,
                    "MtttierInit: failed to initialize generic hash context "
                    "for tier %" PRITier ". Aborting...\n",
                    tier);
            GenericHashReinitialize();
            return false;
        }
    }
    return true;
}

static char ThreeInARow(ReadOnlyString board, const int *indices) {
    if (board[indices[0]] != board[indices[1]]) return 0;
    if (board[indices[1]] != board[indices[2]]) return 0;
    if (board[indices[0]] == '-') return 0;
    return board[indices[0]];
}

static bool AllFilledIn(ReadOnlyString board) {
    for (int i = 0; i < 9; ++i) {
        if (board[i] == '-') return false;
    }
    return true;
}

static void CountPieces(ReadOnlyString board, int *xcount, int *ocount) {
    *xcount = *ocount = 0;
    for (int i = 0; i < 9; ++i) {
        *xcount += (board[i] == 'X');
        *ocount += (board[i] == 'O');
    }
}

static char WhoseTurn(ReadOnlyString board) {
    int xcount, ocount;
    CountPieces(board, &xcount, &ocount);
    if (xcount == ocount) return 'X';  // In our TicTacToe, x always goes first.
    return 'O';
}

static Position DoSymmetry(TierPosition tier_position, int symmetry) {
    char board[9] = {0}, symmetry_board[9] = {0};
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);

    // Copy from the symmetry matrix.
    for (int i = 0; i < 9; ++i) {
        symmetry_board[i] = board[kSymmetryMatrix[symmetry][i]];
    }

    return GenericHashHashLabel(tier_position.tier, symmetry_board, 1);
}

static char ConvertBlankToken(char piece) {
    if (piece == '-') return ' ';
    return piece;
}

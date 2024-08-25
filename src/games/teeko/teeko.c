/**
 * @file teeko.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of Teeko.
 * @details https://en.wikipedia.org/wiki/Teeko
 * @version 1.0.1
 * @date 2024-08-25
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

#include "games/teeko/teeko.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // sprintf, sscanf
#include <stdlib.h>   // atoi
#include <string.h>   // strlen

#include "core/constants.h"
#include "core/generic_hash/generic_hash.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

// ================================= Constants =================================

enum {
    kBoardRows = 5,
    kBoardCols = 5,
    kBoardSize = kBoardRows * kBoardCols,
    kNumStdPatterns = 44,
    kNumExtPatterns = 14,
    kNumSymmetries = 8,
};

static const char kPlayerPiece[3] = {'-', 'X', 'O'};

/**
 * @brief 44 winning patterns in standard game. Grouped in horizontal, vertical,
 * left-to-right oblique, right-to-left oblique, and 2x2 squares.
 */
static const int kPatterns[kNumStdPatterns][4] = {
    {0, 1, 2, 3},     {1, 2, 3, 4},     {5, 6, 7, 8},     {6, 7, 8, 9},
    {10, 11, 12, 13}, {11, 12, 13, 14}, {15, 16, 17, 18}, {16, 17, 18, 19},
    {20, 21, 22, 23}, {21, 22, 23, 24},

    {0, 5, 10, 15},   {5, 10, 15, 20},  {1, 6, 11, 16},   {6, 11, 16, 21},
    {2, 7, 12, 17},   {7, 12, 17, 22},  {3, 8, 13, 18},   {8, 13, 18, 23},
    {4, 9, 14, 19},   {9, 14, 19, 24},

    {1, 7, 13, 19},   {0, 6, 12, 18},   {6, 12, 18, 24},  {5, 11, 17, 23},

    {3, 7, 11, 15},   {4, 8, 12, 16},   {8, 12, 16, 20},  {9, 13, 17, 21},

    {0, 1, 5, 6},     {1, 2, 6, 7},     {2, 3, 7, 8},     {3, 4, 8, 9},
    {5, 6, 10, 11},   {6, 7, 11, 12},   {7, 8, 12, 13},   {8, 9, 13, 14},
    {10, 11, 15, 16}, {11, 12, 16, 17}, {12, 13, 17, 18}, {13, 14, 18, 19},
    {15, 16, 20, 21}, {16, 17, 21, 22}, {17, 18, 22, 23}, {18, 19, 23, 24},
};

/**
 * @brief 14 additional winning patterns in the advanced game variant. Grouped
 * in 3x3, 4x4, and 5x5 squares.
 */
static const int kExtPatterns[kNumExtPatterns][4] = {
    {0, 2, 10, 12},   {1, 3, 11, 13}, {2, 4, 12, 14},   {5, 7, 15, 17},
    {6, 8, 16, 18},   {7, 9, 17, 19}, {10, 12, 20, 22}, {11, 13, 21, 23},
    {12, 14, 22, 24},

    {0, 3, 15, 18},   {1, 4, 16, 19}, {5, 8, 20, 23},   {6, 9, 21, 24},

    {0, 4, 20, 24},
};

/** The symmetry matrix maps indices in the original board to indices in the
 * symmetric boards. */
static int symmetry_matrix[kNumSymmetries][kBoardSize];

// ============================= Variant Settings =============================

static bool advanced = false;

static ConstantReadOnlyString kTeekoWinningRuleChoices[] = {
    "Standard",
    "Advanced",
};

static const GameVariantOption kTeekoWinningRule = {
    .name = "winning rule",
    .num_choices = 2,
    .choices = kTeekoWinningRuleChoices,
};

static GameVariantOption teeko_variant_options[2];
static int teeko_variant_option_selections[2] = {0, 0};  // Standard.

static GameVariant current_variant = {
    .options = teeko_variant_options,
    .selections = teeko_variant_option_selections,
};

// ============================== kTeekoSolverApi ==============================

static Tier TeekoGetInitialTier(void) { return 0; }

static Position TeekoGetInitialPosition(void) {
    static ConstantReadOnlyString kInitialBoard = "-------------------------";
    return GenericHashHashLabel(0, kInitialBoard, 1);
}

static int64_t TeekoGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static Move ConstructMove(int src, int dest) {
    return kBoardSize + src * kBoardSize + dest;
}

static void ExpandMove(Move move, int *src, int *dest) {
    move -= kBoardSize;
    *dest = move % kBoardSize;
    *src = move / kBoardSize;
}

static bool OnBoard(int row, int col) {
    if (row < 0) return false;
    if (col < 0) return false;
    if (row >= kBoardRows) return false;
    if (col >= kBoardCols) return false;

    return true;
}

static void GenerateMovesFrom(const char board[static kBoardSize], int src,
                              MoveArray *moves) {
    int src_row = src / kBoardCols, src_col = src % kBoardCols;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            int dest_row = src_row + i;
            int dest_col = src_col + j;
            if (!OnBoard(dest_row, dest_col)) continue;

            int dest = dest_row * kBoardCols + dest_col;
            if (board[dest] == '-') {
                Move move = ConstructMove(src, dest);
                MoveArrayAppend(moves, move);
            }
        }
    }
}

static MoveArray TeekoGenerateMoves(TierPosition tier_position) {
    Tier tier = tier_position.tier;
    Position pos = tier_position.position;

    // Unhash
    char board[kBoardSize];
    bool success = GenericHashUnhashLabel(tier, pos, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurnLabel(tier, pos);
    char piece_to_move = kPlayerPiece[turn];
    MoveArray ret;
    MoveArrayInit(&ret);

    if (tier < 8) {
        // If in dropping phase, the current player may drop a piece in any
        // empty space.
        for (int i = 0; i < kBoardSize; ++i) {
            if (board[i] == '-') MoveArrayAppend(&ret, i);
        }
    } else {
        // If in moving phase, the current player may move one of their pieces
        // to an adjacent empty space.
        for (int i = 0; i < kBoardSize; ++i) {
            if (board[i] == piece_to_move) {
                GenerateMovesFrom(board, i, &ret);
            }
        }
    }

    return ret;
}

static bool PatternFormed(const char board[static kBoardSize],
                          const int pattern[static 4]) {
    return board[pattern[0]] != '-' && board[pattern[0]] == board[pattern[1]] &&
           board[pattern[0]] == board[pattern[2]] &&
           board[pattern[0]] == board[pattern[3]];
}

static Value TeekoPrimitive(TierPosition tier_position) {
    // No pattern can be formed with both sides having fewer than 4 pieces.
    if (tier_position.tier < 7) return kUndecided;

    // Unhash
    char board[kBoardSize];
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    assert(success);
    (void)success;

    // Check for standard winning patterns.
    for (int i = 0; i < kNumStdPatterns; ++i) {
        if (PatternFormed(board, kPatterns[i])) return kLose;
    }

    // If playing advanced variant, also check for extra winning patterns.
    if (advanced) {
        for (int i = 0; i < kNumExtPatterns; ++i) {
            if (PatternFormed(board, kExtPatterns[i])) return kLose;
        }
    }

    // No pattern is formed, the game continues.
    return kUndecided;
}

static TierPosition TeekoDoMove(TierPosition tier_position, Move move) {
    Tier tier = tier_position.tier;
    TierPosition ret = {.tier = tier < 8 ? tier + 1 : tier};

    // Unhash
    char board[kBoardSize];
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurnLabel(tier, tier_position.position);
    char piece_to_move = kPlayerPiece[turn];
    if (tier < 8) {  // Dropping
        assert(move >= 0 && move < kBoardSize && board[move] == '-');
        board[move] = piece_to_move;
    } else {  // Moving
        int src, dest;
        ExpandMove(move, &src, &dest);
        assert(src >= 0 && dest >= 0 && src < kBoardSize && dest < kBoardSize);
        assert(board[src] == piece_to_move && board[dest] == '-');
        board[dest] = board[src];
        board[src] = '-';
    }
    ret.position = GenericHashHashLabel(ret.tier, board, 3 - turn);

    return ret;
}

static bool TeekoIsLegalPosition(TierPosition tier_position) {
    // Unhash
    char board[kBoardSize];
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    assert(success);
    (void)success;
    int num_patterns = 0;
    // Check for standard winning patterns.
    for (int i = 0; i < kNumStdPatterns; ++i) {
        num_patterns += PatternFormed(board, kPatterns[i]);
    }

    // If playing advanced variant, also check for extra winning patterns.
    if (advanced) {
        for (int i = 0; i < kNumExtPatterns; ++i) {
            num_patterns += PatternFormed(board, kExtPatterns[i]);
        }
    }

    // The players cannot win the game at the same time.
    return num_patterns < 2;
}

static Position TeekoGetCanonicalPosition(TierPosition tier_position) {
    // Unhash
    char board[kBoardSize], symm_board[kBoardSize];
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    assert(success);
    (void)success;
    int turn =
        GenericHashGetTurnLabel(tier_position.tier, tier_position.position);
    Position ret = tier_position.position;
    for (int i = 1; i < kNumSymmetries; ++i) {
        for (int j = 0; j < kBoardSize; ++j) {
            symm_board[j] = board[symmetry_matrix[i][j]];
        }
        Position symm =
            GenericHashHashLabel(tier_position.tier, symm_board, turn);
        if (symm < ret) ret = symm;
    }

    return ret;
}

// static PositionArray TeekoGetCanonicalChildPositions(
//     TierPosition tier_position) {
//     // TODO
// }

static TierArray TeekoGetChildTiers(Tier tier) {
    TierArray ret;
    TierArrayInit(&ret);
    assert(tier >= 0 && tier <= 8);
    if (tier < 8) TierArrayAppend(&ret, tier + 1);

    return ret;
}

static int TeekoGetTierName(Tier tier, char name[static kDbFileNameLengthMax]) {
    assert(tier >= 0 && tier <= 8);
    if (tier < 8) {
        sprintf(name, "%" PRITier "_dropped", tier);
    } else {
        sprintf(name, "moving_phase");
    }

    return kNoError;
}

static const TierSolverApi kTeekoSolverApi = {
    .GetInitialTier = TeekoGetInitialTier,
    .GetInitialPosition = TeekoGetInitialPosition,
    .GetTierSize = TeekoGetTierSize,

    .GenerateMoves = TeekoGenerateMoves,
    .Primitive = TeekoPrimitive,
    .DoMove = TeekoDoMove,
    .IsLegalPosition = TeekoIsLegalPosition,
    .GetCanonicalPosition = TeekoGetCanonicalPosition,
    .GetCanonicalChildPositions = NULL,

    .GetChildTiers = TeekoGetChildTiers,
    .GetTierName = TeekoGetTierName,
};

// ============================= kTeekoGameplayApi =============================

// Simple automatic board string formatting from Teeko.
static int TeekoTierPositionToString(TierPosition tier_position, char *buffer) {
    // Unhash
    char board[kBoardSize];
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    if (!success) return kGenericHashError;

    int offset = 0;
    for (int r = 0; r < kBoardRows; ++r) {
        if (r == (kBoardRows - 1) / 2) {
            offset += sprintf(buffer + offset, "LEGEND: ");
        } else {
            offset += sprintf(buffer + offset, "        ");
        }

        for (int c = 0; c < kBoardCols; ++c) {
            int index = r * kBoardCols + c + 1;
            if (c == 0) {
                offset += sprintf(buffer + offset, "(%2d", index);
            } else {
                offset += sprintf(buffer + offset, " %2d", index);
            }
        }
        offset += sprintf(buffer + offset, ")");

        if (r == (kBoardRows - 1) / 2) {
            offset += sprintf(buffer + offset, "    BOARD: : ");
        } else {
            offset += sprintf(buffer + offset, "           : ");
        }

        for (int c = 0; c < kBoardCols; ++c) {
            int index = r * kBoardCols + c;
            offset += sprintf(buffer + offset, "%c ", board[index]);
        }
        offset += sprintf(buffer + offset, "\n");
    }

    return kNoError;
}

static int TeekoMoveToString(Move move, char *buffer) {
    if (move < kBoardSize) {
        sprintf(buffer, "%" PRIMove, move + 1);
    } else {
        int src, dest;
        ExpandMove(move, &src, &dest);
        sprintf(buffer, "%d %d", src + 1, dest + 1);
    }

    return kNoError;
}

static bool TeekoIsValidMoveString(ReadOnlyString move_string) {
    if (move_string == NULL) return false;

    int length = strlen(move_string);
    if (length < 1 || length > 5) return false;
    int src, dest;
    int num_tokens = sscanf(move_string, "%d %d", &src, &dest);
    if (num_tokens < 0) return false;
    if (num_tokens == 1) {  // Dropping move.
        return src > 0 && src <= kBoardSize;
    }

    // Moving move.
    return src > 0 && src <= kBoardSize && dest > 0 && dest <= kBoardSize;
}

static Move TeekoStringToMove(ReadOnlyString move_string) {
    int src, dest;
    int num_tokens = sscanf(move_string, "%d %d", &src, &dest);
    if (num_tokens < 0) return false;
    if (num_tokens == 1) {  // Dropping move.
        return (Move)src - 1;
    }

    return ConstructMove(src - 1, dest - 1);
}

static const GameplayApiCommon TeekoGameplayApiCommon = {
    .GetInitialPosition = TeekoGetInitialPosition,
    .position_string_length_max = 400,

    .move_string_length_max = 6,
    .MoveToString = TeekoMoveToString,

    .IsValidMoveString = TeekoIsValidMoveString,
    .StringToMove = TeekoStringToMove,
};

static const GameplayApiTier TeekoGameplayApiTier = {
    .GetInitialTier = TeekoGetInitialTier,

    .TierPositionToString = TeekoTierPositionToString,

    .GenerateMoves = TeekoGenerateMoves,
    .DoMove = TeekoDoMove,
    .Primitive = TeekoPrimitive,
};

static const GameplayApi kTeekoGameplayApi = {
    .common = &TeekoGameplayApiCommon,
    .tier = &TeekoGameplayApiTier,
};

// ========================== TeekoGetCurrentVariant ==========================

static const GameVariant *TeekoGetCurrentVariant(void) {
    return &current_variant;
}

// =========================== TeekoSetVariantOption ===========================

static int TeekoSetVariantOption(int option, int selection) {
    // There is only one option in the game, and the selection must be between 0
    // to num_choices - 1 inclusive.
    if (option != 0 || selection < 0 ||
        selection >= kTeekoWinningRule.num_choices) {
        return kRuntimeError;
    }

    teeko_variant_option_selections[0] = selection;
    advanced = (selection == 1);

    return kNoError;
}

// ================================= TeekoInit =================================

/* Rotates the SRC board 90 degrees clockwise and stores resulting board in
 * DEST. */
static void Rotate90(int dest[static kBoardSize], int src[static kBoardSize]) {
    for (int r = 0; r < kBoardRows; r++) {
        for (int c = 0; c < kBoardCols; c++) {
            int new_r = c;
            int new_c = kBoardCols - r - 1;
            dest[new_r * kBoardCols + new_c] = src[r * kBoardCols + c];
        }
    }
}

/* Reflects the SRC board across the middle column and stores resulting board in
 * DEST. */
static void Mirror(int dest[static kBoardSize], int src[static kBoardSize]) {
    for (int r = 0; r < kBoardRows; r++) {
        for (int c = 0; c < kBoardCols; c++) {
            int new_r = r;
            int new_c = kBoardCols - c - 1;
            dest[new_r * kBoardCols + new_c] = src[r * kBoardCols + c];
        }
    }
}

static void InitSymmetricMatrix(void) {
    // Manually fill in the original board index layout.
    for (int i = 0; i < kBoardSize; ++i) {
        symmetry_matrix[0][i] = i;
    }

    // Fill in the symmetric index layouts
    Rotate90(symmetry_matrix[1], symmetry_matrix[0]);  // Rotate 90
    Rotate90(symmetry_matrix[2], symmetry_matrix[1]);  // Rotate 180
    Rotate90(symmetry_matrix[3], symmetry_matrix[2]);  // Rotate 270
    Mirror(symmetry_matrix[4], symmetry_matrix[0]);    // Mirror
    Rotate90(symmetry_matrix[5], symmetry_matrix[4]);  // Rotate mirror 90
    Rotate90(symmetry_matrix[6], symmetry_matrix[5]);  // Rotate mirror 180
    Rotate90(symmetry_matrix[7], symmetry_matrix[6]);  // Rotate mirror 270
}

static int TeekoInit(void *aux) {
    (void)aux;
    teeko_variant_options[0] = kTeekoWinningRule;
    GenericHashReinitialize();
    bool success;

    // Tiers 0 - 7 contain positions in the dropping phase.
    int piece_init[10] = {'X', 0, 0, 'O', 0, 0, '-', 0, 0, -1};
    for (Tier t = 0; t < 8; ++t) {
        piece_init[1] = piece_init[2] = (t + 1) / 2;
        piece_init[4] = piece_init[5] = t / 2;
        piece_init[7] = piece_init[8] =
            kBoardSize - piece_init[1] - piece_init[4];
        success =
            GenericHashAddContext(t % 2 + 1, kBoardSize, piece_init, NULL, t);
        if (!success) return kGenericHashError;
    }

    // Tier 8 contains all positions in the moving phase.
    piece_init[1] = piece_init[2] = 4;
    piece_init[4] = piece_init[5] = 4;
    piece_init[7] = piece_init[8] = kBoardSize - 8;
    success = GenericHashAddContext(0, kBoardSize, piece_init, NULL, 8);
    if (!success) return kGenericHashError;

    InitSymmetricMatrix();

    return TeekoSetVariantOption(0, 0);  // Initialize to standard rule.
}

// =============================== TeekoFinalize ===============================

static int TeekoFinalize(void) { return kNoError; }

// ================================ kTeekoUwapi ================================

static bool TeekoIsLegalFormalPosition(ReadOnlyString formal_position) {
    // Format: "[turn]_[board]"
    if (strlen(formal_position) != 27) return false;
    if (formal_position[0] != '1' && formal_position[0] != '2') return false;
    if (formal_position[1] != '_') return false;

    int num_x = 0, num_o = 0, num_blanks = 0;
    for (int i = 0; i < kBoardSize; ++i) {
        num_x += formal_position[i + 2] == 'X';
        num_o += formal_position[i + 2] == 'O';
        num_blanks += formal_position[i + 2] == '-';
    }
    if (num_x + num_o + num_blanks != 25) return false;
    if (num_x != num_o && num_x != num_o + 1) return false;
    if (num_x + num_o > 8) return false;

    return true;
}

static TierPosition TeekoFormalPositionToTierPosition(
    ReadOnlyString formal_position) {
    // Format: "[turn]_[board]"
    int turn = formal_position[0] - '0';
    // Count the number of pieces on board
    int num_pieces = 0;
    for (int i = 0; i < kBoardSize; ++i) {
        num_pieces += formal_position[i + 2] != '-';
    }
    TierPosition ret;
    ret.tier = num_pieces;
    ret.position = GenericHashHashLabel(num_pieces, formal_position + 2, turn);

    return ret;
}

static CString TeekoTierPositionToFormalPosition(TierPosition tier_position) {
    char formal_position[] = "1_-------------------------";  // placeholder
    int turn =
        GenericHashGetTurnLabel(tier_position.tier, tier_position.position);
    formal_position[0] = turn + '0';
    GenericHashUnhashLabel(tier_position.tier, tier_position.position,
                           formal_position + 2);
    CString ret;
    CStringInitCopy(&ret, formal_position);

    return ret;
}

static CString TeekoTierPositionToAutoGuiPosition(TierPosition tier_position) {
    return TeekoTierPositionToFormalPosition(tier_position);
}

static CString TeekoMoveToFormalMove(TierPosition tier_position, Move move) {
    CString ret;
    char formal_move[kInt32Base10StringLengthMax * 2 + 2];
    if (tier_position.tier < 8) {  // Dropping
        sprintf(formal_move, "%d", (int)move);
    } else {  // Moving
        int src, dest;
        ExpandMove(move, &src, &dest);
        sprintf(formal_move, "%d %d", src, dest);
    }
    CStringInitCopy(&ret, formal_move);

    return ret;
}

static CString TeekoMoveToAutoGuiMove(TierPosition tier_position, Move move) {
    CString ret;
    char autogui_move[kInt32Base10StringLengthMax * 2 + 6];
    if (tier_position.tier < 8) {  // Dropping, A-type move
        sprintf(autogui_move, "A_-_%d_y", (int)move);
    } else {  // Moving, M-type move
        int src, dest;
        ExpandMove(move, &src, &dest);
        sprintf(autogui_move, "M_%d_%d_x", src, dest);
    }
    CStringInitCopy(&ret, autogui_move);

    return ret;
}

static const UwapiTier kTeekoUwapiTier = {
    .GetInitialTier = TeekoGetInitialTier,
    .GetInitialPosition = TeekoGetInitialPosition,
    .GetRandomLegalTierPosition = NULL,

    .GenerateMoves = TeekoGenerateMoves,
    .DoMove = TeekoDoMove,
    .Primitive = TeekoPrimitive,

    .IsLegalFormalPosition = TeekoIsLegalFormalPosition,
    .FormalPositionToTierPosition = TeekoFormalPositionToTierPosition,
    .TierPositionToFormalPosition = TeekoTierPositionToFormalPosition,
    .TierPositionToAutoGuiPosition = TeekoTierPositionToAutoGuiPosition,
    .MoveToFormalMove = TeekoMoveToFormalMove,
    .MoveToAutoGuiMove = TeekoMoveToAutoGuiMove,
};

static const Uwapi kTeekoUwapi = {.tier = &kTeekoUwapiTier};

// ================================== kTeeko ==================================

const Game kTeeko = {
    .name = "teeko",
    .formal_name = "Teeko",
    .solver = &kTierSolver,
    .solver_api = &kTeekoSolverApi,
    .gameplay_api = &kTeekoGameplayApi,
    .uwapi = &kTeekoUwapi,

    .Init = TeekoInit,
    .Finalize = TeekoFinalize,

    .GetCurrentVariant = TeekoGetCurrentVariant,
    .SetVariantOption = TeekoSetVariantOption,
};

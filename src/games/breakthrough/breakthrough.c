/**
 * @file breakthrough.c
 * @author Kelvin Ortiz: ported from the GamesmanPy version (breakthrough.py).
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of Breakthrough.
 * @details https://en.wikipedia.org/wiki/Breakthrough_(board_game)
 *
 * Rules (mirrors breakthrough.py): flat board, index = row * cols + col. 'o'
 * starts on the low rows, moves toward higher rows, and moves first. 'x' starts
 * on the high rows and moves toward lower rows. A piece moves one square
 * forward, either straight onto an empty square or diagonally onto an empty
 * square or an opponent piece (capture). The player to move loses if the
 * opponent has a piece on the mover's home row, or if the mover has no moves.
 *
 * Tiers: (number of 'o' pieces, number of 'x' pieces). A capture moves to a
 * child tier with one fewer opponent piece; every other move stays in the same
 * tier. Pieces only move forward, so every tier is loop-free.
 *
 * Moves use the Python encoding (src << 3) | (dr_bit << 2) | dir, where dir is
 * 0 = straight, 1 = diag-left, 2 = diag-right and dr_bit is 1 iff the piece
 * moves toward higher rows.
 *
 * @version 1.0.0
 * @date 2026-10-05
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

#include "games/breakthrough/breakthrough.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/data_structures/cstring.h"
#include "core/hash/generic.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/base.h"
#include "core/types/database/database.h"
#include "core/types/game/game.h"
#include "core/types/game/game_variant.h"
#include "core/types/gameplay_api/gameplay_api.h"
#include "core/types/gameplay_api/gameplay_api_common.h"
#include "core/types/gameplay_api/gameplay_api_tier.h"
#include "core/types/gamesman_status.h"
#include "core/types/move_array.h"
#include "core/types/uwapi/uwapi.h"
#include "core/types/uwapi/uwapi_tier.h"

// Game, Solver, Gameplay, and UWAPI Functions

static int BreakthroughInit(void *aux);
static int BreakthroughFinalize(void);

static const GameVariant *BreakthroughGetCurrentVariant(void);
static int BreakthroughSetVariantOption(int option, int selection);

static Tier BreakthroughGetInitialTier(void);
static Position BreakthroughGetInitialPosition(void);

static int64_t BreakthroughGetTierSize(Tier tier);
static int BreakthroughGenerateMoves(TierPosition tier_position,
                                     Move moves[static kTierSolverNumMovesMax]);
static Value BreakthroughPrimitive(TierPosition tier_position);
static TierPosition BreakthroughDoMove(TierPosition tier_position, Move move);
static bool BreakthroughIsLegalPosition(TierPosition tier_position);
static int BreakthroughGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier,
    Position parents[static kTierSolverNumParentPositionsMax]);
static int BreakthroughGetChildTiers(
    Tier tier, Tier children[static kTierSolverNumChildTiersMax]);
static TierType BreakthroughGetTierType(Tier tier);
static int BreakthroughGetTierName(Tier tier,
                                  char name[static kDbFileNameLengthMax + 1]);

static MoveArray BreakthroughGenerateMovesGameplay(TierPosition tier_position);
static int BreakthroughTierPositionToString(TierPosition tier_position,
                                            char *buffer);
static int BreakthroughMoveToString(Move move, char *buffer);
static bool BreakthroughIsValidMoveString(ReadOnlyString move_string);
static Move BreakthroughStringToMove(ReadOnlyString move_string);

static bool BreakthroughIsLegalFormalPosition(ReadOnlyString formal_position);
static TierPosition BreakthroughFormalPositionToTierPosition(
    ReadOnlyString formal_position);
static CString BreakthroughTierPositionToFormalPosition(
    TierPosition tier_position);
static CString BreakthroughTierPositionToAutoGuiPosition(
    TierPosition tier_position);
static CString BreakthroughMoveToFormalMove(TierPosition tier_position,
                                            Move move);
static CString BreakthroughMoveToAutoGuiMove(TierPosition tier_position,
                                             Move move);

// Solver API Setup

/** No symmetry functions: positions and tiers are all treated as canonical. */
static const TierSolverApi kSolverApi = {
    .GetInitialTier = &BreakthroughGetInitialTier,
    .GetInitialPosition = &BreakthroughGetInitialPosition,

    .GetTierSize = &BreakthroughGetTierSize,
    .GenerateMoves = &BreakthroughGenerateMoves,
    .Primitive = &BreakthroughPrimitive,
    .DoMove = &BreakthroughDoMove,
    .IsLegalPosition = &BreakthroughIsLegalPosition,
    .GetCanonicalPosition = NULL,
    .GetCanonicalChildPositions = NULL,
    .GetCanonicalParentPositions = &BreakthroughGetCanonicalParentPositions,
    .GetPositionInSymmetricTier = NULL,
    .GetChildTiers = &BreakthroughGetChildTiers,
    .GetTierType = &BreakthroughGetTierType,
    .GetCanonicalTier = NULL,

    .GetTierName = &BreakthroughGetTierName,
};

// Gameplay API Setup

static const GameplayApiCommon kBreakthroughGameplayApiCommon = {
    .GetInitialPosition = &BreakthroughGetInitialPosition,
    .position_string_length_max = 512,

    .move_string_length_max = 8,
    .MoveToString = &BreakthroughMoveToString,

    .IsValidMoveString = &BreakthroughIsValidMoveString,
    .StringToMove = &BreakthroughStringToMove,
};

static const GameplayApiTier kBreakthroughGameplayApiTier = {
    .GetInitialTier = &BreakthroughGetInitialTier,

    .TierPositionToString = &BreakthroughTierPositionToString,

    .GenerateMoves = &BreakthroughGenerateMovesGameplay,
    .DoMove = &BreakthroughDoMove,
    .Primitive = &BreakthroughPrimitive,
};

static const GameplayApi kBreakthroughGameplayApi = {
    .common = &kBreakthroughGameplayApiCommon,
    .tier = &kBreakthroughGameplayApiTier,
};

// UWAPI Setup

static const UwapiTier kBreakthroughUwapiTier = {
    .GenerateMoves = &BreakthroughGenerateMovesGameplay,
    .DoMove = &BreakthroughDoMove,
    .Primitive = &BreakthroughPrimitive,
    .IsLegalFormalPosition = &BreakthroughIsLegalFormalPosition,
    .FormalPositionToTierPosition = &BreakthroughFormalPositionToTierPosition,
    .TierPositionToFormalPosition = &BreakthroughTierPositionToFormalPosition,
    .TierPositionToAutoGuiPosition = &BreakthroughTierPositionToAutoGuiPosition,
    .MoveToFormalMove = &BreakthroughMoveToFormalMove,
    .MoveToAutoGuiMove = &BreakthroughMoveToAutoGuiMove,
    .GetInitialTier = &BreakthroughGetInitialTier,
    .GetInitialPosition = &BreakthroughGetInitialPosition,
    .GetRandomLegalTierPosition = NULL,
};

static const Uwapi kBreakthroughUwapi = {.tier = &kBreakthroughUwapiTier};

const Game kBreakthrough = {
    .name = "breakthrough",
    .formal_name = "Breakthrough",
    .solver = &kTierSolver,
    .solver_api = (const void *)&kSolverApi,
    .gameplay_api = &kBreakthroughGameplayApi,
    .uwapi = &kBreakthroughUwapi,

    .Init = &BreakthroughInit,
    .Finalize = &BreakthroughFinalize,

    .GetCurrentVariant = &BreakthroughGetCurrentVariant,
    .SetVariantOption = &BreakthroughSetVariantOption,
};

// Helper Types and Global Variables

enum {
    kBoardSizeMax = 24,  // Largest variant: 4 cols x 6 rows.
    kTierBase = 256,     // tier = num_o * kTierBase + num_x.
    kStraight = 0,
    kDiagLeft = 1,
    kDiagRight = 2,
};

static const char kEmpty = '-';
static const char kO = 'o';
static const char kX = 'x';

/** Generic hash turn values: 1 = 'o' (moves first), 2 = 'x'. */
static const char kPlayerPiece[3] = {'-', 'o', 'x'};

/** Variant table. Choice names are cols x rows, as in GamesmanPy. */
static ConstantReadOnlyString kBreakthroughSizeChoices[] = {"3x4", "3x5",
                                                            "4x5", "4x6"};
static const int kVariantCols[] = {3, 3, 4, 4};
static const int kVariantRows[] = {4, 5, 5, 6};
static const int kVariantPawnRows[] = {1, 2, 2, 2};

static const GameVariantOption kBreakthroughSize = {
    .name = "size",
    .num_choices = 4,
    .choices = kBreakthroughSizeChoices,
};

// Zero-terminated option list, as required by GameVariant.
static GameVariantOption breakthrough_variant_options[2];
static int breakthrough_variant_option_selections[2] = {0, 0};

static GameVariant current_variant = {
    .options = breakthrough_variant_options,
    .selections = breakthrough_variant_option_selections,
};

// Current variant dimensions.
static int rows;
static int cols;
static int board_size;
static int num_pawns;  // Starting pieces per side.

// Helper Functions

static bool InitGenericHash(void);
static Tier MakeTier(int num_o, int num_x);
static int TierNumO(Tier tier);
static int TierNumX(Tier tier);
static int UnhashBoard(TierPosition tier_position, char *board);
static char Opponent(char player);
static int GetIndex(int row, int col);
static void DecodeMove(Move move, int *src, int *target_row, int *target_col);
static int GetSrcIndex(Move move);
static int GetEndIndex(Move move);
static ReadOnlyString DirectionName(Move move);
static void CountPieces(const char *board, int *num_o, int *num_x);

/** @brief Sets up the default variant (3x4). */
static int BreakthroughInit(void *aux) {
    (void)aux;  // Unused.
    breakthrough_variant_options[0] = kBreakthroughSize;

    return BreakthroughSetVariantOption(0, 0);
}

/** @brief Releases all generic hash contexts. */
static int BreakthroughFinalize(void) {
    GenericHashReinitialize();

    return kSuccess;
}

/** @brief Returns the current variant (board size option). */
static const GameVariant *BreakthroughGetCurrentVariant(void) {
    return &current_variant;
}

/**
 * @brief Switches to board size SELECTION (0 = 3x4, 1 = 3x5, 2 = 4x5,
 * 3 = 4x6) and rebuilds the hash contexts, since the board size changes.
 */
static int BreakthroughSetVariantOption(int option, int selection) {
    // There is only one option, and the selection must be in range.
    if (option != 0 || selection < 0 ||
        selection >= kBreakthroughSize.num_choices) {
        return kRuntimeError;
    }

    breakthrough_variant_option_selections[0] = selection;
    cols = kVariantCols[selection];
    rows = kVariantRows[selection];
    board_size = rows * cols;
    num_pawns = kVariantPawnRows[selection] * cols;

    return InitGenericHash() ? kSuccess : kGenericHashError;
}

/** @brief Returns the tier with all pieces on the board. */
static Tier BreakthroughGetInitialTier(void) {
    return MakeTier(num_pawns, num_pawns);
}

/** @brief Mirrors Breakthrough.start. Assumes generic hash is initialized. */
static Position BreakthroughGetInitialPosition(void) {
    char board[kBoardSizeMax];
    int pawn_rows = num_pawns / cols;
    for (int i = 0; i < board_size; ++i) {
        int row = i / cols;
        if (row < pawn_rows) {
            board[i] = kO;
        } else if (row >= rows - pawn_rows) {
            board[i] = kX;
        } else {
            board[i] = kEmpty;
        }
    }

    return GenericHashHashLabel(BreakthroughGetInitialTier(), board, 1);
}

/** @brief Returns the number of hash values in TIER. */
static int64_t BreakthroughGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

/** @brief Mirrors Breakthrough.generate_moves. */
static int BreakthroughGenerateMoves(
    TierPosition tier_position, Move moves[static kTierSolverNumMovesMax]) {
    char board[kBoardSizeMax];
    int turn = UnhashBoard(tier_position, board);
    char player = kPlayerPiece[turn];
    char other_player = Opponent(player);
    // 'o' moves toward increasing rows, 'x' toward decreasing rows.
    int dr = (player == kO) ? 1 : -1;
    int dr_bit = (dr == 1) ? 1 : 0;

    int ret = 0;
    for (int i = 0; i < board_size; ++i) {
        if (board[i] != player) continue;
        int row = i / cols, col = i % cols;
        int target_row = row + dr;
        if (target_row < 0 || target_row >= rows) continue;

        Move move_base = ((Move)i << 3) | ((Move)dr_bit << 2);
        // Straight: only onto empty, never a capture.
        if (board[GetIndex(target_row, col)] == kEmpty) {
            moves[ret++] = move_base | kStraight;
        }
        // Diagonal: onto empty, or capturing an opponent piece.
        if (col > 0) {
            char target = board[GetIndex(target_row, col - 1)];
            if (target == kEmpty || target == other_player) {
                moves[ret++] = move_base | kDiagLeft;
            }
        }
        if (col < cols - 1) {
            char target = board[GetIndex(target_row, col + 1)];
            if (target == kEmpty || target == other_player) {
                moves[ret++] = move_base | kDiagRight;
            }
        }
    }

    return ret;
}

/**
 * @brief Mirrors Breakthrough.primitive: the player to move loses if the
 * opponent reached the mover's home row or the mover has no moves.
 */
static Value BreakthroughPrimitive(TierPosition tier_position) {
    char board[kBoardSizeMax];
    int turn = UnhashBoard(tier_position, board);
    char player = kPlayerPiece[turn];
    char other_player = Opponent(player);

    int home_row = (player == kO) ? 0 : rows - 1;
    for (int col = 0; col < cols; ++col) {
        if (board[GetIndex(home_row, col)] == other_player) return kLose;
    }

    Move moves[kTierSolverNumMovesMax];
    if (BreakthroughGenerateMoves(tier_position, moves) == 0) return kLose;

    return kUndecided;
}

/**
 * @brief Mirrors Breakthrough.do_move. A capture moves to the child tier with
 * one fewer opponent piece; other moves stay in the same tier.
 */
static TierPosition BreakthroughDoMove(TierPosition tier_position, Move move) {
    char board[kBoardSizeMax];
    int turn = UnhashBoard(tier_position, board);
    int src, target_row, target_col;
    DecodeMove(move, &src, &target_row, &target_col);
    int end = GetIndex(target_row, target_col);

    int num_o = TierNumO(tier_position.tier);
    int num_x = TierNumX(tier_position.tier);
    if (board[end] == kO) --num_o;  // Captured an 'o'.
    if (board[end] == kX) --num_x;  // Captured an 'x'.

    // Overwriting the target cell is the capture; then clear the source.
    board[end] = board[src];
    board[src] = kEmpty;

    TierPosition ret;
    ret.tier = MakeTier(num_o, num_x);
    ret.position = GenericHashHashLabel(ret.tier, board, 3 - turn);

    return ret;
}

/**
 * @brief Weak legality test used only to skip unreachable positions. If the
 * player to move already has a piece on the opponent's home row, the opponent
 * lost on the previous turn and could not have moved, so this is unreachable.
 */
static bool BreakthroughIsLegalPosition(TierPosition tier_position) {
    char board[kBoardSizeMax];
    int turn = UnhashBoard(tier_position, board);
    char player = kPlayerPiece[turn];
    int far_row = (player == kO) ? rows - 1 : 0;
    for (int col = 0; col < cols; ++col) {
        if (board[GetIndex(far_row, col)] == player) return false;
    }

    return true;
}

/**
 * @brief Stores in PARENTS all positions in PARENT_TIER that reach
 * TIER_POSITION in one move, by undoing each possible last move. Needed by the
 * tier solver's backward induction for loop-free tiers.
 */
static int BreakthroughGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier,
    Position parents[static kTierSolverNumParentPositionsMax]) {
    //
    char board[kBoardSizeMax];
    int turn = UnhashBoard(tier_position, board);
    int prev_turn = 3 - turn;
    char prev_player = kPlayerPiece[prev_turn];  // Made the last move.
    char mover = kPlayerPiece[turn];             // Its piece may be captured.

    // Same tier: the last move was not a capture. Otherwise the parent tier
    // must have exactly one more of the mover's pieces (it got captured).
    Tier tier = tier_position.tier;
    bool capture;
    if (parent_tier == tier) {
        capture = false;
    } else if (mover == kO &&
               parent_tier == MakeTier(TierNumO(tier) + 1, TierNumX(tier))) {
        capture = true;
    } else if (mover == kX &&
               parent_tier == MakeTier(TierNumO(tier), TierNumX(tier) + 1)) {
        capture = true;
    } else {
        return 0;
    }

    int dr = (prev_player == kO) ? 1 : -1;
    int ret = 0;
    for (int end = 0; end < board_size; ++end) {
        if (board[end] != prev_player) continue;
        int src_row = end / cols - dr;
        if (src_row < 0 || src_row >= rows) continue;

        int end_col = end % cols;
        for (int src_col = end_col - 1; src_col <= end_col + 1; ++src_col) {
            if (src_col < 0 || src_col >= cols) continue;
            if (capture && src_col == end_col) continue;  // No straight capture.
            int src = GetIndex(src_row, src_col);
            if (board[src] != kEmpty) continue;

            // Undo the move, hash the parent, then restore the board.
            board[src] = prev_player;
            board[end] = capture ? mover : kEmpty;
            parents[ret++] = GenericHashHashLabel(parent_tier, board, prev_turn);
            board[src] = kEmpty;
            board[end] = prev_player;
        }
    }

    return ret;
}

/** @brief A capture removes one piece of either side, if both have pieces. */
static int BreakthroughGetChildTiers(
    Tier tier, Tier children[static kTierSolverNumChildTiersMax]) {
    //
    int num_o = TierNumO(tier), num_x = TierNumX(tier);
    if (num_o == 0 || num_x == 0) return 0;  // Nobody can capture.

    children[0] = MakeTier(num_o - 1, num_x);
    children[1] = MakeTier(num_o, num_x - 1);

    return 2;
}

/** @brief Pieces only move forward, so no position repeats within a tier. */
static TierType BreakthroughGetTierType(Tier tier) {
    (void)tier;  // All tiers are loop-free.
    return kTierTypeLoopFree;
}

/** @brief Writes the tier name, e.g. "3o_2x". */
static int BreakthroughGetTierName(Tier tier,
                                   char name[static kDbFileNameLengthMax + 1]) {
    snprintf(name, kDbFileNameLengthMax + 1, "%do_%dx", TierNumO(tier),
             TierNumX(tier));

    return kSuccess;
}

/** @brief GenerateMoves wrapper that returns a MoveArray for gameplay/UWAPI. */
static MoveArray BreakthroughGenerateMovesGameplay(TierPosition tier_position) {
    Move moves[kTierSolverNumMovesMax];
    int num_moves = BreakthroughGenerateMoves(tier_position, moves);
    MoveArray ret;
    MoveArrayInit(&ret);
    for (int i = 0; i < num_moves; ++i) {
        MoveArrayAppend(&ret, moves[i]);
    }

    return ret;
}

/**
 * @brief Text board for the terminal: index legend on the left, board on the
 * right, highest row first (like Python's TUI mode), then turn and tier.
 */
static int BreakthroughTierPositionToString(TierPosition tier_position,
                                            char *buffer) {
    char board[kBoardSizeMax];
    if (!GenericHashUnhashLabel(tier_position.tier, tier_position.position,
                                board)) {
        return kGenericHashError;
    }
    int turn = GenericHashGetTurnLabel(tier_position.tier,
                                       tier_position.position);

    int size = kBreakthroughGameplayApiCommon.position_string_length_max + 1;
    int offset = 0;
    for (int row = rows - 1; row >= 0; --row) {
        offset += snprintf(buffer + offset, size - offset, "        (");
        for (int col = 0; col < cols; ++col) {
            offset += snprintf(buffer + offset, size - offset, " %2d",
                               GetIndex(row, col));
        }
        offset += snprintf(buffer + offset, size - offset, " )    ");
        for (int col = 0; col < cols; ++col) {
            offset += snprintf(buffer + offset, size - offset, " %c",
                               board[GetIndex(row, col)]);
        }
        offset += snprintf(buffer + offset, size - offset, "\n");
    }
    char tier_name[kDbFileNameLengthMax + 1];
    BreakthroughGetTierName(tier_position.tier, tier_name);
    offset += snprintf(buffer + offset, size - offset, "Turn: %c    Tier: %s",
                       kPlayerPiece[turn], tier_name);

    if (offset >= size) {
        fprintf(stderr,
                "BreakthroughTierPositionToString: (BUG) not enough space was "
                "allocated to buffer. Please increase "
                "position_string_length_max.\n");
        return kBufferOverflowError;
    }

    return kSuccess;
}

/**
 * @brief Terminal move string "{src}-{end}", e.g. "4-7". The formal
 * "{src}-{direction}" string cannot be parsed back without the position (it
 * does not say which way the piece moves), so gameplay uses the end index.
 */
static int BreakthroughMoveToString(Move move, char *buffer) {
    int src = GetSrcIndex(move), end = GetEndIndex(move);
    int actual_length =
        snprintf(buffer, kBreakthroughGameplayApiCommon.move_string_length_max + 1,
                 "%d-%d", src, end);
    if (actual_length >=
        kBreakthroughGameplayApiCommon.move_string_length_max + 1) {
        fprintf(stderr,
                "BreakthroughMoveToString: (BUG) not enough space was "
                "allocated to buffer. Please increase "
                "move_string_length_max.\n");
        return kBufferOverflowError;
    }

    return kSuccess;
}

/** @brief Parses "{src}-{end}" into SRC and END. Returns false if malformed. */
static bool ParseMoveString(ReadOnlyString move_string, int *src, int *end) {
    if (move_string == NULL) return false;
    int length = (int)strlen(move_string);
    if (length < 3 || length > 5) return false;

    // Only digits and exactly one '-' that is neither first nor last.
    int dash = -1;
    for (int i = 0; i < length; ++i) {
        if (move_string[i] == '-' && dash < 0) {
            dash = i;
        } else if (move_string[i] < '0' || move_string[i] > '9') {
            return false;
        }
    }
    if (dash <= 0 || dash == length - 1) return false;

    return sscanf(move_string, "%d-%d", src, end) == 2;
}

/** @brief True if the string is "{src}-{end}" with END one row from SRC. */
static bool BreakthroughIsValidMoveString(ReadOnlyString move_string) {
    int src, end;
    if (!ParseMoveString(move_string, &src, &end)) return false;
    if (src < 0 || src >= board_size || end < 0 || end >= board_size) {
        return false;
    }

    int row_diff = end / cols - src / cols;
    int col_diff = end % cols - src % cols;
    if (row_diff != 1 && row_diff != -1) return false;
    if (col_diff < -1 || col_diff > 1) return false;

    return true;
}

/** @brief Converts "{src}-{end}" back to the Python move encoding. */
static Move BreakthroughStringToMove(ReadOnlyString move_string) {
    assert(BreakthroughIsValidMoveString(move_string));
    int src, end;
    ParseMoveString(move_string, &src, &end);

    int dr_bit = (end / cols > src / cols) ? 1 : 0;
    int col_diff = end % cols - src % cols;
    int dir = kStraight;
    if (col_diff == -1) dir = kDiagLeft;
    if (col_diff == 1) dir = kDiagRight;

    return ((Move)src << 3) | ((Move)dr_bit << 2) | dir;
}

/**
 * @brief Strictly validates an untrusted formal position: board characters
 * '-', 'o', 'x', a trailing turn character 'o' or 'x', piece counts within the
 * variant, and not an unreachable position.
 */
static bool BreakthroughIsLegalFormalPosition(ReadOnlyString formal_position) {
    if (formal_position == NULL) return false;
    if ((int)strlen(formal_position) != board_size + 1) return false;

    for (int i = 0; i < board_size; ++i) {
        char c = formal_position[i];
        if (c != kEmpty && c != kO && c != kX) return false;
    }
    char turn = formal_position[board_size];
    if (turn != kO && turn != kX) return false;

    int num_o, num_x;
    CountPieces(formal_position, &num_o, &num_x);
    if (num_o > num_pawns || num_x > num_pawns) return false;

    TierPosition tier_position =
        BreakthroughFormalPositionToTierPosition(formal_position);

    return BreakthroughIsLegalPosition(tier_position);
}

/** @brief Formal position (board + turn char) to tier position. */
static TierPosition BreakthroughFormalPositionToTierPosition(
    ReadOnlyString formal_position) {
    //
    int num_o, num_x;
    CountPieces(formal_position, &num_o, &num_x);
    int turn = (formal_position[board_size] == kO) ? 1 : 2;

    TierPosition ret;
    ret.tier = MakeTier(num_o, num_x);
    ret.position = GenericHashHashLabel(ret.tier, formal_position, turn);

    return ret;
}

/** @brief Board + turn char, e.g. "ooo------xxxo" (Python's readable string). */
static CString BreakthroughTierPositionToFormalPosition(
    TierPosition tier_position) {
    //
    CString ret = {0};
    char formal[kBoardSizeMax + 2];
    int turn = UnhashBoard(tier_position, formal);
    formal[board_size] = kPlayerPiece[turn];
    formal[board_size + 1] = '\0';
    CStringInitCopyCharArray(&ret, formal);

    return ret;
}

/** @brief "1_" + board on o's turn, "2_" + board on x's turn. */
static CString BreakthroughTierPositionToAutoGuiPosition(
    TierPosition tier_position) {
    //
    CString ret = {0};
    char autogui[kBoardSizeMax + 3];
    int turn = UnhashBoard(tier_position, autogui + 2);
    autogui[0] = (char)('0' + turn);
    autogui[1] = '_';
    autogui[board_size + 2] = '\0';
    CStringInitCopyCharArray(&ret, autogui);

    return ret;
}

/** @brief "{src}-{straight|diag-left|diag-right}", as in Python. */
static CString BreakthroughMoveToFormalMove(TierPosition tier_position,
                                            Move move) {
    (void)tier_position;  // The move alone is enough.
    CString ret = {0};
    char formal[32];
    snprintf(formal, sizeof(formal), "%d-%s", GetSrcIndex(move),
             DirectionName(move));
    CStringInitCopyCharArray(&ret, formal);

    return ret;
}

/** @brief "M_{src}_{end}_x", as in Python. */
static CString BreakthroughMoveToAutoGuiMove(TierPosition tier_position,
                                             Move move) {
    (void)tier_position;  // The move alone is enough.
    CString ret = {0};
    char autogui[32];
    snprintf(autogui, sizeof(autogui), "M_%d_%d_x", GetSrcIndex(move),
             GetEndIndex(move));
    CStringInitCopyCharArray(&ret, autogui);

    return ret;
}

// Helper functions implementation

/**
 * @brief Creates one two-player hash context per tier (num_o, num_x), each
 * holding exactly num_o 'o's and num_x 'x's.
 */
static bool InitGenericHash(void) {
    GenericHashReinitialize();
    int player = 0;  // Both players: the hash stores whose turn it is.
    for (int num_o = 0; num_o <= num_pawns; ++num_o) {
        for (int num_x = 0; num_x <= num_pawns; ++num_x) {
            int num_empty = board_size - num_o - num_x;
            int pieces_init_array[10] = {kEmpty, num_empty, num_empty,
                                         kO,     num_o,     num_o,
                                         kX,     num_x,     num_x,
                                         -1};
            Tier tier = MakeTier(num_o, num_x);
            bool success = GenericHashAddContext(player, board_size,
                                                 pieces_init_array, NULL, tier);
            if (!success) {
                fprintf(stderr,
                        "BreakthroughInit: failed to initialize generic hash "
                        "context for tier %" PRITier ". Aborting...\n",
                        tier);
                GenericHashReinitialize();
                return false;
            }
        }
    }

    return true;
}

/** @brief Encodes piece counts as one Tier value. */
static Tier MakeTier(int num_o, int num_x) {
    return (Tier)num_o * kTierBase + num_x;
}

/** @brief Number of 'o' pieces in TIER. */
static int TierNumO(Tier tier) { return (int)(tier / kTierBase); }

/** @brief Number of 'x' pieces in TIER. */
static int TierNumX(Tier tier) { return (int)(tier % kTierBase); }

/** @brief Unhashes TIER_POSITION into BOARD and returns the turn (1 or 2). */
static int UnhashBoard(TierPosition tier_position, char *board) {
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    assert(success);
    (void)success;

    return GenericHashGetTurnLabel(tier_position.tier, tier_position.position);
}

/** @brief Mirrors Breakthrough._opponent. */
static char Opponent(char player) { return (player == kO) ? kX : kO; }

/** @brief Mirrors Breakthrough.get_index. */
static int GetIndex(int row, int col) { return row * cols + col; }

/** @brief Mirrors Breakthrough._decode_move. */
static void DecodeMove(Move move, int *src, int *target_row, int *target_col) {
    int direction = (int)(move & 3);
    int dr = ((move >> 2) & 1) ? 1 : -1;
    *src = GetSrcIndex(move);
    *target_row = *src / cols + dr;
    *target_col = *src % cols;
    if (direction == kDiagLeft) --(*target_col);
    if (direction == kDiagRight) ++(*target_col);
}

/** @brief Board index the moved piece starts on. */
static int GetSrcIndex(Move move) { return (int)(move >> 3); }

/** @brief Board index the moved piece lands on. */
static int GetEndIndex(Move move) {
    int src, target_row, target_col;
    DecodeMove(move, &src, &target_row, &target_col);

    return GetIndex(target_row, target_col);
}

/** @brief Direction name used in formal moves. */
static ReadOnlyString DirectionName(Move move) {
    int direction = (int)(move & 3);
    if (direction == kDiagLeft) return "diag-left";
    if (direction == kDiagRight) return "diag-right";

    return "straight";
}

/** @brief Counts 'o' and 'x' pieces in the first board_size cells of BOARD. */
static void CountPieces(const char *board, int *num_o, int *num_x) {
    *num_o = *num_x = 0;
    for (int i = 0; i < board_size; ++i) {
        *num_o += (board[i] == kO);
        *num_x += (board[i] == kX);
    }
}

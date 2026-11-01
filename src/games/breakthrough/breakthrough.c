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
 * Goal-row-aware tiers and hashing. While the game is still going, no 'o' can
 * be on the top row and no 'x' on the bottom row (reaching it ends the game).
 * Positions are grouped into tiers (o, x, g):
 *   o, x = number of 'o' and 'x' pieces,
 *   g    = 0 game still going (either player to move),
 *          1 an 'o' just reached the top row ('x' to move, and has lost),
 *          2 an 'x' just reached the bottom row ('o' to move, and has lost).
 * On the biggest boards ("finer tiers"), each tier is further split by
 *   a    = number of 'o' pieces on the bottom row,
 *   b    = number of 'x' pieces on the top row,
 * so the largest tier is much smaller and fits in memory. A piece that leaves
 * its home row never returns, so finer tiers are still loop-free.
 * Inside a tier, the board is split into three regions (bottom row, middle
 * rows, top row). Boards are grouped by a = 'o's on the bottom row and b =
 * 'x's on the top row; the hash is the group's offset (BIAS) plus the board's
 * rank inside the group, where each region is ranked as a multiset
 * permutation and the three ranks are combined (see the GamesCrafters hashing
 * slides). Only boards that obey the goal-row rule get a hash value, so far
 * fewer slots are wasted than with a plain piece-count hash. Pieces only move
 * forward, so every tier is loop-free.
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

// Optimizations, all ON by default. Each can be switched off at compile time
// (used for benchmarks). Databases are only readable by a build with the same
// mirror / color-swap settings as the build that solved them.
//   BREAKTHROUGH_NO_FAST_CHILDREN  turn OFF fast child generation; the solver
//                                  then falls back to GenerateMoves + DoMove +
//                                  a hash set (also turns off the mirror).
//   BREAKTHROUGH_NO_MIRROR         turn OFF left-right mirror symmetry.
//   BREAKTHROUGH_NO_COLOR_SWAP     turn OFF color-swap tier symmetry: flipping
//                                  the board upside down and swapping 'o' with
//                                  'x' (and the turn) gives an equivalent
//                                  position, so only one tier of each pair is
//                                  solved and stored.
// Finer tiers (splitting each tier by how many pieces are still on their home
// row) are on only for the boards marked in kVariantFineTiers, so the
// databases of the other boards stay readable. For benchmarks:
//   BREAKTHROUGH_FINE_TIERS_ALL    turn ON finer tiers for every board.
//   BREAKTHROUGH_NO_FINE_TIERS     turn OFF finer tiers for every board.
#ifdef BREAKTHROUGH_NO_FAST_CHILDREN
#define BT_FAST_CHILDREN 0
#else
#define BT_FAST_CHILDREN 1
#endif
#if !defined(BREAKTHROUGH_NO_MIRROR) && BT_FAST_CHILDREN
#define BT_MIRROR 1
#else
#define BT_MIRROR 0
#endif
#ifndef BREAKTHROUGH_NO_COLOR_SWAP
#define BT_COLOR_SWAP 1
#else
#define BT_COLOR_SWAP 0
#endif

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/data_structures/cstring.h"
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
#if BT_FAST_CHILDREN
static int BreakthroughGetNumberOfCanonicalChildPositions(
    TierPosition tier_position);
static int BreakthroughGetCanonicalChildPositions(
    TierPosition tier_position,
    TierPosition children[static kTierSolverNumChildPositionsMax]);
#endif
#if BT_COLOR_SWAP
static Tier BreakthroughGetCanonicalTier(Tier tier);
static Position BreakthroughGetPositionInSymmetricTier(TierPosition tier_position,
                                                       Tier symmetric);
#endif
#if BT_MIRROR
static Position BreakthroughGetCanonicalPosition(TierPosition tier_position);
static int BreakthroughGetNumberOfSymmetries(TierPosition tier_position);
#endif
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
#if BT_MIRROR
    .GetCanonicalPosition = &BreakthroughGetCanonicalPosition,
    .GetNumberOfSymmetries = &BreakthroughGetNumberOfSymmetries,
#endif
#if BT_FAST_CHILDREN
    .GetNumberOfCanonicalChildPositions =
        &BreakthroughGetNumberOfCanonicalChildPositions,
    .GetCanonicalChildPositions = &BreakthroughGetCanonicalChildPositions,
#endif
    .GetCanonicalParentPositions = &BreakthroughGetCanonicalParentPositions,
#if BT_COLOR_SWAP
    .GetPositionInSymmetricTier = &BreakthroughGetPositionInSymmetricTier,
#endif
    .GetChildTiers = &BreakthroughGetChildTiers,
    .GetTierType = &BreakthroughGetTierType,
#if BT_COLOR_SWAP
    .GetCanonicalTier = &BreakthroughGetCanonicalTier,
#endif

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
    kBoardSizeMax = 36,  // Room for boards up to 6 x 6.
    kColsMax = 6,
    kPawnsMax = 18,  // Three rows of pawns on a 6-wide board.
    kStraight = 0,
    kDiagLeft = 1,
    kDiagRight = 2,
};

static const char kEmpty = '-';
static const char kO = 'o';
static const char kX = 'x';

/** Turn values: 1 = 'o' (moves first), 2 = 'x'. */
static const char kPlayerPiece[3] = {'-', 'o', 'x'};

/** Variant table. Choice names are cols x rows, as in GamesmanPy. */
static ConstantReadOnlyString kBreakthroughSizeChoices[] = {
    "3x4", "3x5", "4x5", "4x6", "5x5", "5x6",
};
static const int kVariantCols[] = {3, 3, 4, 4, 5, 5};
static const int kVariantRows[] = {4, 5, 5, 6, 5, 6};
static const int kVariantPawnRows[] = {1, 2, 2, 2, 2, 2};
// Finer tiers (see "Tiers" above) only for boards whose biggest tier would
// not fit in memory otherwise. Changing this for a board makes its existing
// databases unreadable, so smaller boards keep the coarser tiers.
static const bool kVariantFineTiers[] = {false, false, false, false, false,
                                         true};

static const GameVariantOption kBreakthroughSize = {
    .name = "size",
    .num_choices = 6,
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
static bool fine_tiers;  // Split tiers by goal-row groups (big boards).

// Helper Functions

static void InitHashTables(void);
static Tier MakeTier(int o, int x, int g);
static Tier MakeFineTier(int o, int x, int g, int a, int b);
static void SplitTier(Tier tier, int *o, int *x, int *g);
static bool SplitFineTier(Tier tier, int *a, int *b);
#if BT_COLOR_SWAP
static Tier ColorSwapTier(Tier tier);
#endif
static int64_t RegionCount(int size, int num_o, int num_x);
static bool RegionCounts(int o, int x, int g, int a, int b, int counts[3][2]);
static int64_t TierSize(Tier tier);
static bool TierFromBoard(const char *board, int turn, Tier *tier);
static bool BoardGroup(const char *board, int turn, Tier *tier, int *a,
                       int *b);
static TierPosition HashBoard(const char *board, int turn);
static const char *CanonicalBoard(const char *board, char *buffer);
#if BT_COLOR_SWAP
static void ColorSwapBoard(const char *board, char *swapped);
#endif
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

/** @brief Nothing to release: the hash tables are static. */
static int BreakthroughFinalize(void) { return kSuccess; }

/** @brief Returns the current variant (board size option). */
static const GameVariant *BreakthroughGetCurrentVariant(void) {
    return &current_variant;
}

/**
 * @brief Switches to board size SELECTION (0 = 3x4, 1 = 3x5, 2 = 4x5,
 * 3 = 4x6, 4 = 5x5, 5 = 5x6) and sets the board dimensions used by the hash.
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
#if defined(BREAKTHROUGH_NO_FINE_TIERS)
    fine_tiers = false;
    (void)kVariantFineTiers;
#elif defined(BREAKTHROUGH_FINE_TIERS_ALL)
    fine_tiers = true;
    (void)kVariantFineTiers;
#else
    fine_tiers = kVariantFineTiers[selection];
#endif

    InitHashTables();

    return kSuccess;
}

/** @brief Returns the starting tier: all pieces on the board, game going. */
static Tier BreakthroughGetInitialTier(void) {
    if (fine_tiers) return MakeFineTier(num_pawns, num_pawns, 0, cols, cols);
    return MakeTier(num_pawns, num_pawns, 0);
}

/** @brief Mirrors Breakthrough.start. */
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

    return HashBoard(board, 1).position;
}

/** @brief Returns the number of hash values in TIER. */
static int64_t BreakthroughGetTierSize(Tier tier) { return TierSize(tier); }

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
 * @brief Mirrors Breakthrough.do_move. The child's tier is worked out from the
 * resulting board (piece counts, goal-row pieces, whether the game just ended).
 */
static TierPosition BreakthroughDoMove(TierPosition tier_position, Move move) {
    char board[kBoardSizeMax];
    int turn = UnhashBoard(tier_position, board);
    int src, target_row, target_col;
    DecodeMove(move, &src, &target_row, &target_col);
    int end = GetIndex(target_row, target_col);

    // Overwriting the target cell is the capture; then clear the source.
    board[end] = board[src];
    board[src] = kEmpty;

    return HashBoard(board, 3 - turn);
}

/**
 * @brief Every hash value now stands for a board that obeys the goal-row rule
 * (the hash leaves out boards where the player to move already reached its
 * goal row), so there is nothing left to filter here.
 */
static bool BreakthroughIsLegalPosition(TierPosition tier_position) {
    (void)tier_position;
    return true;
}

#if BT_FAST_CHILDREN
/**
 * @brief Fast child generation: unhashes once, applies every move on a copy
 * of the board and hashes the result. With color swap on, a child in a
 * non-canonical tier is stored as its color-swapped twin in the canonical tier
 * (matching the solver's default child generation). Different moves always
 * give different boards, so duplicates can only appear when a symmetry is on
 * (two children that are mirror images or color swaps of each other).
 */
static int BreakthroughGetCanonicalChildPositions(
    TierPosition tier_position,
    TierPosition children[static kTierSolverNumChildPositionsMax]) {
    //
    char board[kBoardSizeMax], child[kBoardSizeMax], buffer[kBoardSizeMax];
    int turn = UnhashBoard(tier_position, board);
    char player = kPlayerPiece[turn];
    int dr = (player == kO) ? 1 : -1;

    int ret = 0;
    for (int i = 0; i < board_size; ++i) {
        if (board[i] != player) continue;
        int target_row = i / cols + dr;
        if (target_row < 0 || target_row >= rows) continue;
        int col = i % cols;
        for (int dc = -1; dc <= 1; ++dc) {
            int target_col = col + dc;
            if (target_col < 0 || target_col >= cols) continue;
            int end = GetIndex(target_row, target_col);
            char target = board[end];
            if (target == player) continue;
            if (dc == 0 && target != kEmpty) continue;  // No straight capture.

            memcpy(child, board, board_size);
            child[end] = player;
            child[i] = kEmpty;
            int child_turn = 3 - turn;
            const char *stored = child;
#if BT_COLOR_SWAP
            // Store the child in its canonical tier, like the solver's own
            // default child generation does.
            char swapped[kBoardSizeMax];
            Tier child_tier;
            TierFromBoard(child, child_turn, &child_tier);
            if (BreakthroughGetCanonicalTier(child_tier) != child_tier) {
                ColorSwapBoard(child, swapped);
                stored = swapped;
                child_turn = 3 - child_turn;
            }
#endif
            TierPosition c =
                HashBoard(CanonicalBoard(stored, buffer), child_turn);
#if BT_MIRROR || BT_COLOR_SWAP
            bool duplicate = false;
            for (int k = 0; k < ret; ++k) {
                if (children[k].tier == c.tier &&
                    children[k].position == c.position) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;
#endif
            children[ret++] = c;
        }
    }

    return ret;
}

/** @brief Number of unique (canonical) children. */
static int BreakthroughGetNumberOfCanonicalChildPositions(
    TierPosition tier_position) {
    //
#if BT_MIRROR || BT_COLOR_SWAP
    TierPosition children[kTierSolverNumChildPositionsMax];
    return BreakthroughGetCanonicalChildPositions(tier_position, children);
#else
    // Without symmetry every move gives a distinct child.
    Move moves[kTierSolverNumMovesMax];
    return BreakthroughGenerateMoves(tier_position, moves);
#endif
}
#endif  // BT_FAST_CHILDREN

#if BT_MIRROR
/**
 * @brief Returns the hash of whichever of TIER_POSITION and its left-right
 * mirror image is smaller as a board string. Mirroring keeps the piece counts
 * and the goal-row groups, so both are in the same tier.
 */
static Position BreakthroughGetCanonicalPosition(TierPosition tier_position) {
    char board[kBoardSizeMax], buffer[kBoardSizeMax];
    int turn = UnhashBoard(tier_position, board);
    const char *canonical = CanonicalBoard(board, buffer);
    if (canonical == board) return tier_position.position;

    return HashBoard(canonical, turn).position;
}

/** @brief 1 if the board is its own mirror image, 2 otherwise. */
static int BreakthroughGetNumberOfSymmetries(TierPosition tier_position) {
    char board[kBoardSizeMax], buffer[kBoardSizeMax];
    UnhashBoard(tier_position, board);
    CanonicalBoard(board, buffer);  // Fills BUFFER with the mirror image.

    return memcmp(board, buffer, board_size) == 0 ? 1 : 2;
}
#endif  // BT_MIRROR

/**
 * @brief Undoes every possible last move on BOARD (TURN to move now) and adds
 * each parent whose board belongs to PARENT_TIER to PARENTS, skipping
 * duplicates. Returns the new number of parents.
 */
static int CollectParents(char *board, int turn, Tier parent_tier,
                          Position *parents, int ret) {
    int prev_turn = 3 - turn;
    char prev_player = kPlayerPiece[prev_turn];  // Made the last move.
    char mover = kPlayerPiece[turn];             // Its piece may be captured.
    int dr = (prev_player == kO) ? 1 : -1;

    for (int end = 0; end < board_size; ++end) {
        if (board[end] != prev_player) continue;
        int src_row = end / cols - dr;
        if (src_row < 0 || src_row >= rows) continue;

        int end_col = end % cols;
        for (int src_col = end_col - 1; src_col <= end_col + 1; ++src_col) {
            if (src_col < 0 || src_col >= cols) continue;
            int src = GetIndex(src_row, src_col);
            if (board[src] != kEmpty) continue;

            // Option 0: plain move. Option 1: capture (diagonal only).
            for (int capture = 0; capture <= 1; ++capture) {
                if (capture && src_col == end_col) continue;
                board[src] = prev_player;
                board[end] = capture ? mover : kEmpty;
                Tier tier;
                if (TierFromBoard(board, prev_turn, &tier) &&
                    tier == parent_tier) {
                    char buffer[kBoardSizeMax];
                    Position parent =
                        HashBoard(CanonicalBoard(board, buffer), prev_turn)
                            .position;
                    bool duplicate = false;
                    for (int k = 0; k < ret; ++k) {
                        if (parents[k] == parent) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (!duplicate) parents[ret++] = parent;
                }
                board[src] = kEmpty;
                board[end] = prev_player;
            }
        }
    }

    return ret;
}

/**
 * @brief Stores in PARENTS all positions in PARENT_TIER that reach
 * TIER_POSITION in one move. Each possible last move is undone (both as a
 * plain move and, for diagonal moves, as a capture); a parent is kept only if
 * its board belongs to PARENT_TIER. With mirror symmetry on, each parent is
 * stored in its canonical form. With color swap on, the solver passes the
 * child in its stored (canonical) tier, but the real child of PARENT_TIER may
 * be its color-swapped twin, so moves are undone from both versions.
 */
static int BreakthroughGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier,
    Position parents[static kTierSolverNumParentPositionsMax]) {
    //
    char board[kBoardSizeMax];
    int turn = UnhashBoard(tier_position, board);
    int ret = CollectParents(board, turn, parent_tier, parents, 0);

#if BT_COLOR_SWAP
    if (ColorSwapTier(tier_position.tier) != tier_position.tier) {
        char swapped[kBoardSizeMax];
        ColorSwapBoard(board, swapped);
        ret = CollectParents(swapped, 3 - turn, parent_tier, parents, ret);
    }
#endif

    return ret;
}

#if BT_COLOR_SWAP
/**
 * @brief Color swap: tier (o, x, g) is equivalent to tier (x, o, g'), where
 * g' swaps "an 'o' just won" (1) with "an 'x' just won" (2). Returns the
 * smaller of the two as the canonical tier.
 */
static Tier BreakthroughGetCanonicalTier(Tier tier) {
    Tier swapped = ColorSwapTier(tier);
    return swapped < tier ? swapped : tier;
}

/**
 * @brief Returns the position in tier SYMMETRIC that is the color swap of
 * TIER_POSITION: the board flipped upside down, 'o' and 'x' swapped, and the
 * other player to move.
 */
static Position BreakthroughGetPositionInSymmetricTier(TierPosition tier_position,
                                                       Tier symmetric) {
    if (symmetric == tier_position.tier) return tier_position.position;

    char board[kBoardSizeMax], swapped[kBoardSizeMax];
    int turn = UnhashBoard(tier_position, board);
    ColorSwapBoard(board, swapped);
    TierPosition ret = HashBoard(swapped, 3 - turn);
    assert(ret.tier == symmetric);

    return ret.position;
}
#endif  // BT_COLOR_SWAP

/**
 * @brief Lists the tiers one move can lead to. Only tiers where the game is
 * still going (g = 0) have moves. A move can capture an enemy piece and/or
 * reach the goal row (ending the game); every other move stays in the same
 * tier. Tiers with no valid boards are skipped.
 */
static int BreakthroughGetChildTiers(
    Tier tier, Tier children[static kTierSolverNumChildTiersMax]) {
    //
    int o, x, g;
    SplitTier(tier, &o, &x, &g);
    if (g != 0) return 0;  // The game is over: no moves.

    Tier candidates[12];
    int n = 0;
    int a, b;
    if (SplitFineTier(tier, &a, &b)) {
        // Finer tiers also change when a piece leaves its home row.
        candidates[n++] = MakeFineTier(o, x - 1, 0, a, b);      // o captures
        candidates[n++] = MakeFineTier(o, x, 0, a - 1, b);      // o leaves home
        candidates[n++] = MakeFineTier(o, x - 1, 0, a - 1, b);  // both
        candidates[n++] = MakeFineTier(o, x, 1, a, b);          // o reaches top
        candidates[n++] = MakeFineTier(o, x - 1, 1, a, b - 1);  // ...capturing
        candidates[n++] = MakeFineTier(o - 1, x, 0, a, b);      // x captures
        candidates[n++] = MakeFineTier(o, x, 0, a, b - 1);      // x leaves home
        candidates[n++] = MakeFineTier(o - 1, x, 0, a, b - 1);  // both
        candidates[n++] = MakeFineTier(o, x, 2, a, b);          // x reaches bottom
        candidates[n++] = MakeFineTier(o - 1, x, 2, a - 1, b);  // ...capturing
    } else {
        candidates[n++] = MakeTier(o, x - 1, 0);  // 'o' captures in the middle.
        candidates[n++] = MakeTier(o, x, 1);      // 'o' reaches the top row.
        candidates[n++] = MakeTier(o, x - 1, 1);  // ...with a capture.
        candidates[n++] = MakeTier(o - 1, x, 0);  // 'x' captures in the middle.
        candidates[n++] = MakeTier(o, x, 2);      // 'x' reaches the bottom row.
        candidates[n++] = MakeTier(o - 1, x, 2);  // ...with a capture.
    }

    int ret = 0;
    for (int i = 0; i < n; ++i) {
        if (candidates[i] < 0 || candidates[i] == tier) continue;
        if (TierSize(candidates[i]) == 0) continue;
        children[ret++] = candidates[i];
    }

    return ret;
}

/** @brief Pieces only move forward, so no position repeats within a tier. */
static TierType BreakthroughGetTierType(Tier tier) {
    (void)tier;  // All tiers are loop-free.
    return kTierTypeLoopFree;
}

/**
 * @brief Writes the tier name, e.g. "3o_2x_g0": 3 'o's, 2 'x's, game still
 * going ("g1": an 'o' just won, "g2": an 'x' just won).
 */
static int BreakthroughGetTierName(Tier tier,
                                   char name[static kDbFileNameLengthMax + 1]) {
    int o, x, g, a, b;
    SplitTier(tier, &o, &x, &g);
    if (SplitFineTier(tier, &a, &b)) {
        snprintf(name, kDbFileNameLengthMax + 1, "%do_%dx_a%d_b%d_g%d", o, x, a,
                 b, g);
    } else {
        snprintf(name, kDbFileNameLengthMax + 1, "%do_%dx_g%d", o, x, g);
    }

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
    int turn = UnhashBoard(tier_position, board);

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

    // The board must obey the goal-row rule for the player to move.
    Tier tier;
    return TierFromBoard(formal_position, turn == kO ? 1 : 2, &tier);
}

/** @brief Formal position (board + turn char) to tier position. */
static TierPosition BreakthroughFormalPositionToTierPosition(
    ReadOnlyString formal_position) {
    //
    int turn = (formal_position[board_size] == kO) ? 1 : 2;

    return HashBoard(formal_position, turn);
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

// ---------------------------------------------------------------------------
// Goal-row-aware hashing.
//
// The board is split into three regions: the bottom row (row 0), the middle
// rows (1 .. rows-2) and the top row (rows-1). Inside a tier (o, x, g), boards
// are grouped by (a, b) = ('o's on the bottom row, 'x's on the top row), which
// fixes how many 'o's and 'x's each region holds. Each region is ranked as a
// multiset permutation of '-', 'o', 'x' (in that order), exactly like the
// combinatorial hash in the GamesCrafters hashing slides, and
//     hash = offset[a][b] + (rank_bottom * count_middle + rank_middle)
//                            * count_top + rank_top
// where offset[a][b] (the BIAS) counts the boards in all earlier groups. While
// the game is still going (g = 0) the hash is doubled to store the turn.
// ---------------------------------------------------------------------------

enum { kRegionBottom = 0, kRegionMiddle = 1, kRegionTop = 2 };

static int64_t binomial[kBoardSizeMax + 1][kBoardSizeMax + 1];

// group_offset[o][x][g][a][b]: number of boards in groups before (a, b).
// group_count[o][x][g][a][b]: number of boards in group (a, b).
// tier_boards[o][x][g]: number of boards in the tier (without the turn).
static int64_t group_offset[kPawnsMax + 1][kPawnsMax + 1][3][kColsMax + 1]
                          [kColsMax + 1];
static int64_t group_count[kPawnsMax + 1][kPawnsMax + 1][3][kColsMax + 1]
                         [kColsMax + 1];
static int64_t tier_boards[kPawnsMax + 1][kPawnsMax + 1][3];

/** @brief Ways to arrange NUM_O 'o's and NUM_X 'x's in SIZE cells. */
static int64_t RegionCount(int size, int num_o, int num_x) {
    if (num_o < 0 || num_x < 0 || num_o + num_x > size) return 0;
    return binomial[size][num_o] * binomial[size - num_o][num_x];
}

/**
 * @brief Writes how many 'o's (counts[r][0]) and 'x's (counts[r][1]) region r
 * holds in group (a, b) of tier (o, x, g). Returns false if impossible.
 */
static bool RegionCounts(int o, int x, int g, int a, int b, int counts[3][2]) {
    int goal_o = (g == 1), goal_x = (g == 2);  // Piece that just arrived.
    counts[kRegionBottom][0] = a;
    counts[kRegionBottom][1] = goal_x;
    counts[kRegionTop][0] = goal_o;
    counts[kRegionTop][1] = b;
    counts[kRegionMiddle][0] = o - a - goal_o;
    counts[kRegionMiddle][1] = x - b - goal_x;
    int sizes[3] = {cols, board_size - 2 * cols, cols};
    for (int r = 0; r < 3; ++r) {
        if (RegionCount(sizes[r], counts[r][0], counts[r][1]) == 0) {
            return false;
        }
    }
    return true;
}

/** @brief Fills Pascal's triangle and the group offset tables. */
static void InitHashTables(void) {
    for (int n = 0; n <= kBoardSizeMax; ++n) {
        binomial[n][0] = binomial[n][n] = 1;
        for (int k = 1; k < n; ++k) {
            binomial[n][k] = binomial[n - 1][k - 1] + binomial[n - 1][k];
        }
        for (int k = n + 1; k <= kBoardSizeMax; ++k) binomial[n][k] = 0;
    }

    int sizes[3] = {cols, board_size - 2 * cols, cols};
    for (int o = 0; o <= num_pawns; ++o) {
        for (int x = 0; x <= num_pawns; ++x) {
            for (int g = 0; g < 3; ++g) {
                int64_t total = 0;
                for (int a = 0; a <= cols; ++a) {
                    for (int b = 0; b <= cols; ++b) {
                        int counts[3][2];
                        int64_t count = 0;
                        if (RegionCounts(o, x, g, a, b, counts)) {
                            count = 1;
                            for (int r = 0; r < 3; ++r) {
                                count *= RegionCount(sizes[r], counts[r][0],
                                                     counts[r][1]);
                            }
                        }
                        group_offset[o][x][g][a][b] = total;
                        group_count[o][x][g][a][b] = count;
                        total += count;
                    }
                }
                tier_boards[o][x][g] = total;
            }
        }
    }
}

/** @brief Packs (o, x, g) into one Tier value. Returns -1 if invalid. */
static Tier MakeTier(int o, int x, int g) {
    if (o < 0 || x < 0 || o > num_pawns || x > num_pawns || g < 0 || g > 2) {
        return -1;
    }
    return ((Tier)o * 64 + x) * 4 + g;
}

// Finer tiers use their own range of Tier values (from kFineTierBase up), so
// coarse tier values, and the databases that use them, never change.
static const Tier kFineTierBase = (Tier)1 << 32;

/** @brief Packs (o, x, g, a, b) into one finer Tier value; -1 if invalid. */
static Tier MakeFineTier(int o, int x, int g, int a, int b) {
    Tier coarse = MakeTier(o, x, g);
    if (coarse < 0 || a < 0 || b < 0 || a > cols || b > cols) return -1;
    return kFineTierBase + (coarse * 8 + a) * 8 + b;
}

/** @brief Unpacks a Tier value into (o, x, g); works for both kinds. */
static void SplitTier(Tier tier, int *o, int *x, int *g) {
    if (tier >= kFineTierBase) tier = (tier - kFineTierBase) / 64;
    *g = (int)(tier % 4);
    tier /= 4;
    *x = (int)(tier % 64);
    *o = (int)(tier / 64);
}

/** @brief For a finer tier, writes (a, b) and returns true. */
static bool SplitFineTier(Tier tier, int *a, int *b) {
    if (tier < kFineTierBase) return false;
    tier -= kFineTierBase;
    *b = (int)(tier % 8);
    *a = (int)((tier / 8) % 8);
    return true;
}

#if BT_COLOR_SWAP
/**
 * @brief Color-swapped twin of TIER: (o, x, g) -> (x, o, g'), where g' swaps
 * "an 'o' just won" with "an 'x' just won"; for finer tiers a and b swap too
 * ('o's at home on the bottom become 'x's at home on the top).
 */
static Tier ColorSwapTier(Tier tier) {
    int o, x, g, a, b;
    SplitTier(tier, &o, &x, &g);
    int swapped_g = g == 1 ? 2 : (g == 2 ? 1 : 0);
    if (SplitFineTier(tier, &a, &b)) return MakeFineTier(x, o, swapped_g, b, a);
    return MakeTier(x, o, swapped_g);
}
#endif  // BT_COLOR_SWAP

/** @brief Number of hash values in TIER (0 if the tier is impossible). */
static int64_t TierSize(Tier tier) {
    if (tier < 0) return 0;
    int o, x, g, a, b;
    SplitTier(tier, &o, &x, &g);
    if (o > num_pawns || x > num_pawns) return 0;
    int64_t boards = SplitFineTier(tier, &a, &b) ? group_count[o][x][g][a][b]
                                                 : tier_boards[o][x][g];
    return (g == 0) ? boards * 2 : boards;  // g = 0 stores the turn.
}

/**
 * @brief Works out the tier and group (a, b) of BOARD with TURN to move.
 * Returns false if the board breaks the goal-row rule (e.g. the player to move
 * already reached its goal row, or both players did).
 */
static bool BoardGroup(const char *board, int turn, Tier *tier, int *a,
                       int *b) {
    int o = 0, x = 0, o_bottom = 0, x_bottom = 0, o_top = 0, x_top = 0;
    int top_start = board_size - cols;
    for (int i = 0; i < board_size; ++i) {
        if (board[i] == kO) {
            ++o;
            if (i < cols) ++o_bottom;
            if (i >= top_start) ++o_top;
        } else if (board[i] == kX) {
            ++x;
            if (i < cols) ++x_bottom;
            if (i >= top_start) ++x_top;
        }
    }
    if (o > num_pawns || x > num_pawns) return false;

    int g;
    if (o_top == 0 && x_bottom == 0) {
        g = 0;
    } else if (o_top == 1 && x_bottom == 0 && turn == 2) {
        g = 1;  // 'o' just arrived; 'x' to move.
    } else if (x_bottom == 1 && o_top == 0 && turn == 1) {
        g = 2;  // 'x' just arrived; 'o' to move.
    } else {
        return false;
    }
    *tier = fine_tiers ? MakeFineTier(o, x, g, o_bottom, x_top)
                       : MakeTier(o, x, g);
    *a = o_bottom;
    *b = x_top;
    return true;
}

/** @brief Works out the tier of BOARD with TURN to move (see BoardGroup). */
static bool TierFromBoard(const char *board, int turn, Tier *tier) {
    int a, b;
    return BoardGroup(board, turn, tier, &a, &b);
}

/** @brief Cells of region R, in increasing board index. Returns the size. */
static int RegionCells(int r, int *first) {
    if (r == kRegionBottom) {
        *first = 0;
        return cols;
    }
    if (r == kRegionMiddle) {
        *first = cols;
        return board_size - 2 * cols;
    }
    *first = board_size - cols;
    return cols;
}

/** @brief Rank of the SIZE cells starting at CELLS among all arrangements. */
static int64_t RankRegion(const char *cells, int size, int num_o, int num_x) {
    int64_t rank = 0;
    int num_empty = size - num_o - num_x;
    for (int i = 0; i < size; ++i) {
        int left = size - i - 1;  // Cells after this one.
        if (cells[i] == kEmpty) {
            --num_empty;
        } else if (cells[i] == kO) {
            // Skip all arrangements that put '-' here.
            if (num_empty > 0) rank += RegionCount(left, num_o, num_x);
            --num_o;
        } else {
            // Skip all arrangements that put '-' or 'o' here.
            if (num_empty > 0) rank += RegionCount(left, num_o, num_x);
            if (num_o > 0) rank += RegionCount(left, num_o - 1, num_x);
            --num_x;
        }
    }
    return rank;
}

/** @brief Inverse of RankRegion: writes the arrangement with rank RANK. */
static void UnrankRegion(int64_t rank, char *cells, int size, int num_o,
                         int num_x) {
    int num_empty = size - num_o - num_x;
    for (int i = 0; i < size; ++i) {
        int left = size - i - 1;
        int64_t with_empty =
            num_empty > 0 ? RegionCount(left, num_o, num_x) : 0;
        if (rank < with_empty) {
            cells[i] = kEmpty;
            --num_empty;
            continue;
        }
        rank -= with_empty;
        int64_t with_o = num_o > 0 ? RegionCount(left, num_o - 1, num_x) : 0;
        if (rank < with_o) {
            cells[i] = kO;
            --num_o;
            continue;
        }
        rank -= with_o;
        cells[i] = kX;
        --num_x;
    }
}

/**
 * @brief Hashes BOARD with TURN to move. Assumes the board obeys the goal-row
 * rule (true for every board reached by legal play).
 */
static TierPosition HashBoard(const char *board, int turn) {
    TierPosition ret;
    int a, b;
    bool ok = BoardGroup(board, turn, &ret.tier, &a, &b);
    assert(ok);
    (void)ok;
    int o, x, g;
    SplitTier(ret.tier, &o, &x, &g);

    int counts[3][2];
    RegionCounts(o, x, g, a, b, counts);
    int64_t rank = 0;
    for (int r = 0; r < 3; ++r) {
        int first;
        int size = RegionCells(r, &first);
        rank = rank * RegionCount(size, counts[r][0], counts[r][1]) +
               RankRegion(board + first, size, counts[r][0], counts[r][1]);
    }
    if (!fine_tiers) rank += group_offset[o][x][g][a][b];  // Group BIAS.
    if (g == 0) rank = rank * 2 + (turn - 1);  // Store the turn.
    ret.position = rank;

    return ret;
}

/** @brief Unhashes TIER_POSITION into BOARD and returns the turn (1 or 2). */
static int UnhashBoard(TierPosition tier_position, char *board) {
    int o, x, g;
    SplitTier(tier_position.tier, &o, &x, &g);
    int64_t rank = tier_position.position;
    int turn;
    if (g == 0) {
        turn = (int)(rank % 2) + 1;
        rank /= 2;
    } else {
        turn = (g == 1) ? 2 : 1;  // The player who did not just move.
    }

    // Find the group (a, b): stored in a finer tier, otherwise the group
    // whose range contains RANK.
    int a = 0, b = 0;
    if (!SplitFineTier(tier_position.tier, &a, &b)) {
        bool found = false;
        for (a = 0; a <= cols && !found; ++a) {
            for (b = 0; b <= cols; ++b) {
                int64_t offset = group_offset[o][x][g][a][b];
                if (rank >= offset &&
                    rank < offset + group_count[o][x][g][a][b]) {
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        assert(found);
        rank -= group_offset[o][x][g][a][b];
    }

    int counts[3][2];
    RegionCounts(o, x, g, a, b, counts);
    for (int r = 2; r >= 0; --r) {
        int first;
        int size = RegionCells(r, &first);
        int64_t count = RegionCount(size, counts[r][0], counts[r][1]);
        UnrankRegion(rank % count, board + first, size, counts[r][0],
                     counts[r][1]);
        rank /= count;
    }

    return turn;
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

/**
 * @brief With mirror symmetry on, returns BOARD or its left-right mirror image
 * (written into BUFFER), whichever is smaller as a string. Otherwise returns
 * BOARD unchanged.
 */
static const char *CanonicalBoard(const char *board, char *buffer) {
#if BT_MIRROR
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            buffer[GetIndex(row, col)] = board[GetIndex(row, cols - 1 - col)];
        }
    }
    return memcmp(board, buffer, board_size) <= 0 ? board : buffer;
#else
    (void)buffer;
    return board;
#endif
}

#if BT_COLOR_SWAP
/** @brief Board flipped upside down with 'o' and 'x' swapped. */
static void ColorSwapBoard(const char *board, char *swapped) {
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            char piece = board[GetIndex(rows - 1 - row, col)];
            swapped[GetIndex(row, col)] =
                piece == kO ? kX : (piece == kX ? kO : kEmpty);
        }
    }
}
#endif  // BT_COLOR_SWAP

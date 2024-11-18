/**
 * @file quixo.c
 * @author Robert Shi (robertyishi@berkeley.edu),
 *         Maria Rufova,
 *         Benji Xu,
 *         Angela He,
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Quixo implementation.
 * @version 1.0.2
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

#include "games/quixo/quixo.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // fprintf, stderr, sprintf
#include <stdlib.h>   // atoi
#include <string.h>   // memset

#include "core/constants.h"
#include "core/generic_hash/generic_hash.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

static int QuixoInit(void *aux);
static int InitGenericHash(void);
static bool IsValidPieceConfig(int num_blanks, int num_x, int num_o);
static Tier HashTier(int num_blanks, int num_x, int num_o);
static int QuixoFinalize(void);

static const GameVariant *QuixoGetCurrentVariant(void);
static int QuixoSetVariantOption(int option, int selection);

static Tier QuixoGetInitialTier(void);
static Position QuixoGetInitialPosition(void);

static int64_t QuixoGetTierSize(Tier tier);
static MoveArray QuixoGenerateMoves(TierPosition tier_position);
static Value QuixoPrimitive(TierPosition tier_position);
static TierPosition QuixoDoMove(TierPosition tier_position, Move move);
static bool QuixoIsLegalPosition(TierPosition tier_position);
static Position QuixoGetCanonicalPosition(TierPosition tier_position);
static int QuixoGetNumberOfCanonicalChildPositions(TierPosition tier_position);
static TierPositionArray QuixoGetCanonicalChildPositions(
    TierPosition tier_position);
static PositionArray QuixoGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier);
static TierArray QuixoGetChildTiers(Tier tier);
static int QuixoGetTierName(Tier tier, char name[static kDbFileNameLengthMax + 1]);

// Gameplay
static int QuixoTierPositionToString(TierPosition tier_position, char *buffer);
static int QuixoMoveToString(Move move, char *buffer);
static bool QuixoIsValidMoveString(ReadOnlyString move_string);
static Move QuixoStringToMove(ReadOnlyString move_string);

// UWAPI
static bool QuixoIsLegalFormalPosition(ReadOnlyString formal_position);
static TierPosition QuixoFormalPositionToTierPosition(
    ReadOnlyString formal_position);
static CString QuixoTierPositionToFormalPosition(TierPosition tier_position);
static CString QuixoTierPositionToAutoGuiPosition(TierPosition tier_position);
static CString QuixoMoveToFormalMove(TierPosition tier_position, Move move);
static CString QuixoMoveToAutoGuiMove(TierPosition tier_position, Move move);

// Variants

static ConstantReadOnlyString kQuixoRuleChoices[] = {
    "5x5 5-in-a-row", "4x4 4-in-a-row", "3x3 3-in-a-row",
    "2x2 2-in-a-row", "4x5 4-in-a-row",
};

static const GameVariantOption kQuixoRules = {
    .name = "rules",
    .num_choices = sizeof(kQuixoRuleChoices) / sizeof(kQuixoRuleChoices[0]),
    .choices = kQuixoRuleChoices,
};

#define NUM_OPTIONS 2  // 1 option and 1 zero-terminator
static GameVariantOption quixo_variant_options[NUM_OPTIONS];
static int quixo_variant_option_selections[NUM_OPTIONS] = {0, 0};  // 5x5
#undef NUM_OPTIONS
static GameVariant current_variant = {
    .options = quixo_variant_options,
    .selections = quixo_variant_option_selections,
};

// Solver API Setup
static const TierSolverApi kQuixoSolverApi = {
    .GetInitialTier = &QuixoGetInitialTier,
    .GetInitialPosition = &QuixoGetInitialPosition,

    .GetTierSize = &QuixoGetTierSize,
    .GenerateMoves = &QuixoGenerateMoves,
    .Primitive = &QuixoPrimitive,
    .DoMove = &QuixoDoMove,
    .IsLegalPosition = &QuixoIsLegalPosition,
    .GetCanonicalPosition = &QuixoGetCanonicalPosition,
    .GetNumberOfCanonicalChildPositions =
        &QuixoGetNumberOfCanonicalChildPositions,
    .GetCanonicalChildPositions = &QuixoGetCanonicalChildPositions,
    .GetCanonicalParentPositions = &QuixoGetCanonicalParentPositions,
    .GetChildTiers = &QuixoGetChildTiers,
    .GetTierName = &QuixoGetTierName,
};

// -----------------------------------------------------------------------------
// Gameplay API Setup

static const GameplayApiCommon QuixoGameplayApiCommon = {
    .GetInitialPosition = &QuixoGetInitialPosition,
    .position_string_length_max = 400,

    .move_string_length_max = 6,
    .MoveToString = &QuixoMoveToString,

    .IsValidMoveString = &QuixoIsValidMoveString,
    .StringToMove = &QuixoStringToMove,
};

static const GameplayApiTier QuixoGameplayApiTier = {
    .GetInitialTier = &QuixoGetInitialTier,

    .TierPositionToString = &QuixoTierPositionToString,

    .GenerateMoves = &QuixoGenerateMoves,
    .DoMove = &QuixoDoMove,
    .Primitive = &QuixoPrimitive,
};

static const GameplayApi QuixoGameplayApi = {
    .common = &QuixoGameplayApiCommon,
    .tier = &QuixoGameplayApiTier,
};

// -----------------------------------------------------------------------------
// UWAPI Setup

static const UwapiTier kQuixoUwapiTier = {
    .GetInitialTier = &QuixoGetInitialTier,
    .GetInitialPosition = &QuixoGetInitialPosition,
    .GetRandomLegalTierPosition = NULL,

    .GenerateMoves = &QuixoGenerateMoves,
    .DoMove = &QuixoDoMove,
    .Primitive = &QuixoPrimitive,

    .IsLegalFormalPosition = &QuixoIsLegalFormalPosition,
    .FormalPositionToTierPosition = &QuixoFormalPositionToTierPosition,
    .TierPositionToFormalPosition = &QuixoTierPositionToFormalPosition,
    .TierPositionToAutoGuiPosition = &QuixoTierPositionToAutoGuiPosition,
    .MoveToFormalMove = &QuixoMoveToFormalMove,
    .MoveToAutoGuiMove = &QuixoMoveToAutoGuiMove,
};

static const Uwapi kQuixoUwapi = {.tier = &kQuixoUwapiTier};
// -----------------------------------------------------------------------------

const Game kQuixo = {
    .name = "quixo",
    .formal_name = "Quixo",
    .solver = &kTierSolver,
    .solver_api = (const void *)&kQuixoSolverApi,
    .gameplay_api = (const GameplayApi *)&QuixoGameplayApi,
    .uwapi = &kQuixoUwapi,

    .Init = &QuixoInit,
    .Finalize = &QuixoFinalize,

    .GetCurrentVariant = &QuixoGetCurrentVariant,
    .SetVariantOption = &QuixoSetVariantOption,
};

// -----------------------------------------------------------------------------

enum QuixoPieces { kBlank = '-', kX = 'X', kO = 'O' };

enum QuixoConstants {
    kBoardRowsMax = 6,
    kBoardColsMax = 6,
    kBoardSizeMax = kBoardRowsMax * kBoardColsMax,
};

static const char kPlayerPiece[3] = {kBlank, kX, kO};  // Cached turn-to-piece
                                                       // mapping.
static Tier initial_tier;
static Position initial_position;
static int board_rows;  // (option) Number of rows on the board (default 5).
static int board_cols;  // (option) Number of columns on the board (default 5).
static int k_in_a_row;  // (option) Number of pieces in a row a player needs
                        // to win.
static int num_edge_slots;  // (calculated) Number of edge slots on the board.
// (calculated) Indices of edge slots. Only the first num_edge_slots entries are
// initialized and used.
static int edge_indices[kBoardSizeMax];
// (calculated) Indices of move destinations of edge pieces. Each edge piece can
// have at most 3 distinct destinations. Only the first num_edge_slots entries
// in the first dimension and the first edge_move_count[i] entries in the second
// dimension are initialized and used. E.g., if board is 5x5 (board_rows ==
// board_cols == 5), edge_indices[0] == 0, and edge_move_dests[0] would
// therefore be {4, 20}.
static int edge_move_dests[kBoardSizeMax][3];
// (calculated) Number of move destinations of each edge slot. In the same
// example from above, edge_move_count[0] is 2, and edge_move_count[1] is 3
// (assuming slot index 1 at row 0 column 1 is of index 1 in the edge_indices
// array.)
static int edge_move_count[kBoardSizeMax];
static int symmetry_matrix[8][kBoardSizeMax];  // (calculated) Symmetry matrix.

// Helper functions for tier initialization.
static void UpdateEdgeSlots(void);
static int GetMoveDestinations(int *dests, int src);
static int GetBoardSize(void);
static Tier QuixoSetInitialTier(void);
static Position QuixoSetInitialPosition(void);
static Tier HashTier(int num_blanks, int num_x, int num_o);
static void UnhashTier(Tier tier, int *num_blanks, int *num_x, int *num_o);
static void QuixoInitSymmMatrix(void);

// Helper functions for QuixoGenerateMoves
static int OpponentsTurn(int turn);
static Move ConstructMove(int src, int dest);
static void UnpackMove(Move move, int *src, int *dest);
static int BoardRowColToIndex(int row, int col);
static void BoardIndexToRowCol(int index, int *row, int *col);

// Helper functions for QuixoPrimitive
static bool HasKInARow(const char *board, char piece);
static bool CheckDirection(const char *board, char piece, int i, int j,
                           int dir_i, int dir_j);

// Symmetry transformation functions
static void Rotate90(int *dest, int *src);
static void Mirror(int *dest, int *src);

// Helper functions for QuixoDoMove and QuixoGetCanonicalParentPositions
static int GetShiftOffset(int src, int dest);

static int QuixoInit(void *aux) {
    (void)aux;  // Unused.
    quixo_variant_options[0] = kQuixoRules;
    return QuixoSetVariantOption(0, 0);  // Initialize the default variant.
}

static int InitGenericHash(void) {
    GenericHashReinitialize();

    // Default to two players. Some tiers can only be one of the two
    // players' turns. Those tiers are dealt with in the for loop.
    static const int kTwoPlayerInitializer = 0;
    int board_size = GetBoardSize();
    // These values are just place holders. The actual values are generated
    // in the for loop below.
    int pieces_init_array[10] = {kBlank, 0, 0, kX, 0, 0, kO, 0, 0, -1};

    bool success;
    for (int num_blanks = 0; num_blanks <= board_size; ++num_blanks) {
        for (int num_x = 0; num_x <= board_size; ++num_x) {
            for (int num_o = 0; num_o <= board_size; ++num_o) {
                if (!IsValidPieceConfig(num_blanks, num_x, num_o)) continue;
                Tier tier = HashTier(num_blanks, num_x, num_o);
                pieces_init_array[1] = pieces_init_array[2] = num_blanks;
                pieces_init_array[4] = pieces_init_array[5] = num_x;
                pieces_init_array[7] = pieces_init_array[8] = num_o;
                success =
                    GenericHashAddContext(kTwoPlayerInitializer, GetBoardSize(),
                                          pieces_init_array, NULL, tier);
                if (!success) {
                    fprintf(
                        stderr,
                        "InitGenericHash: failed to initialize generic hash "
                        "context for tier %" PRITier ". Aborting...\n",
                        tier);
                    GenericHashReinitialize();
                    return kRuntimeError;
                }
            }
        }
    }

    return kNoError;
}

static bool IsValidPieceConfig(int num_blanks, int num_x, int num_o) {
    int board_size = GetBoardSize();
    if (num_blanks < 0 || num_x < 0 || num_o < 0) return false;
    if (num_blanks + num_x + num_o != board_size) return false;

    return true;
}

static int QuixoFinalize(void) {
    GenericHashReinitialize();
    return kNoError;
}

static const GameVariant *QuixoGetCurrentVariant(void) {
    return &current_variant;
}

static int QuixoInitVariant(int rows, int cols, int k) {
    board_rows = rows;
    board_cols = cols;
    k_in_a_row = k;
    UpdateEdgeSlots();
    int ret = InitGenericHash();
    if (ret != kNoError) return ret;

    QuixoSetInitialTier();
    QuixoSetInitialPosition();
    QuixoInitSymmMatrix();

    return kNoError;
}

static int QuixoSetVariantOption(int option, int selection) {
    // There is only one option in the game, and the selection must be between 0
    // to num_choices - 1 inclusive.
    if (option != 0 || selection < 0 || selection >= kQuixoRules.num_choices) {
        return kRuntimeError;
    }

    quixo_variant_option_selections[0] = selection;
    switch (selection) {
        case 0:  // 5x5 5-in-a-row
            return QuixoInitVariant(5, 5, 5);
        case 1:  // 4x4 4-in-a-row
            return QuixoInitVariant(4, 4, 4);
        case 2:  // 3x3 3-in-a-row
            return QuixoInitVariant(3, 3, 3);
        case 3:  // 2x2 2-in-a-row
            return QuixoInitVariant(2, 2, 2);
        case 4:  // 4x5 4-in-a-row
            return QuixoInitVariant(4, 5, 4);
        default:
            return kIllegalGameVariantError;
    }

    return kNoError;
}

static Tier QuixoGetInitialTier(void) { return initial_tier; }

static Position QuixoGetInitialPosition(void) { return initial_position; }

static int64_t QuixoGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static MoveArray QuixoGenerateMoves(TierPosition tier_position) {
    MoveArray moves;
    MoveArrayInit(&moves);
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) {
        moves.size = -1;
        return moves;
    }

    int turn = GenericHashGetTurnLabel(tier, position);
    char piece_to_move = kPlayerPiece[turn];
    // Find all blank or friendly pieces on 4 edges and 4 corners of the board.
    for (int i = 0; i < num_edge_slots; ++i) {
        int edge_index = edge_indices[i];
        char piece = board[edge_index];
        // Skip opponent pieces, which cannot be moved.
        if (piece != kBlank && piece != piece_to_move) continue;

        for (int j = 0; j < edge_move_count[i]; ++j) {
            Move move = ConstructMove(edge_index, edge_move_dests[i][j]);
            MoveArrayAppend(&moves, move);
        }
    }

    return moves;
}

static Value QuixoPrimitive(TierPosition tier_position) {
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) return kErrorValue;

    int turn = GenericHashGetTurnLabel(tier, position);
    assert(turn == 1 || turn == 2);
    char my_piece = kPlayerPiece[turn];
    char opponent_piece = kPlayerPiece[OpponentsTurn(turn)];

    if (HasKInARow(board, my_piece)) {
        // The current player wins if there is a k in a row of the current
        // player's piece, regardless of whether there is a k in a row of the
        // opponent's piece.
        return kWin;
    } else if (HasKInARow(board, opponent_piece)) {
        // If the current player is not winning but there's a k in a row of the
        // opponent's piece, then the current player loses.
        return kLose;
    }

    // Neither side is winning.
    return kUndecided;
}

static int Abs(int n) { return n >= 0 ? n : -n; }

static void MoveAndShiftPieces(char *board, int src, int dest,
                               char piece_to_move) {
    int offset = GetShiftOffset(src, dest);
    for (int i = src; i != dest; i += offset) {
        board[i] = board[i + offset];
    }
    board[dest] = piece_to_move;
}

static Tier GetChildTier(Tier parent, int turn, bool flip) {
    if (!flip) return parent;

    int num_blanks, num_x, num_o;
    UnhashTier(parent, &num_blanks, &num_x, &num_o);
    --num_blanks;
    num_x += (kPlayerPiece[turn] == kX);
    num_o += (kPlayerPiece[turn] == kO);

    return HashTier(num_blanks, num_x, num_o);
}

static TierPosition QuixoDoMove(TierPosition tier_position, Move move) {
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) return kIllegalTierPosition;

    // Get turn and piece for current player.
    int turn = GenericHashGetTurnLabel(tier, position);
    assert(turn == 1 || turn == 2);
    char piece_to_move = kPlayerPiece[turn];

    // Unpack move.
    int src, dest;
    UnpackMove(move, &src, &dest);
    assert(src >= 0 && src < GetBoardSize());
    assert(dest >= 0 && dest < GetBoardSize());
    assert(src != dest);

    // Update tier.
    TierPosition ret = {.tier = GetChildTier(tier, turn, board[src] == kBlank)};

    // Move and shift pieces.
    MoveAndShiftPieces(board, src, dest, piece_to_move);
    ret.position = GenericHashHashLabel(ret.tier, board, OpponentsTurn(turn));

    return ret;
}

// Helper function for shifting a row or column.
static int GetShiftOffset(int src, int dest) {
    // 1 for row move, 0 for column move
    int isRowMove = Abs(dest - src) < board_cols;
    // 1 if dest > src, -1 if dest < src
    int direction = (dest > src) - (dest < src);
    // 1 for right, -1 for left, 0 for column moves
    int rowOffset = direction * isRowMove;
    // board_cols for down, -board_cols for up, 0 for row moves
    int colOffset = direction * (board_cols) * (!isRowMove);
    int offset = rowOffset + colOffset;
    assert(Abs(src - dest) % Abs(offset) == 0);

    return offset;
}

// Returns if a pos is legal, but not strictly according to game definition.
// In X's turn, returns illegal if no border Os, and vice versa
// Will not misidentify legal as illegal, but might misidentify illegal as
// legal.
static bool QuixoIsLegalPosition(TierPosition tier_position) {
    Tier tier = tier_position.tier;              // get tier
    Position position = tier_position.position;  // get pos
    if (tier == initial_tier && position == initial_position) {
        // The initial position is always legal but does not follow the rules
        // below.
        return true;
    }

    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) {
        fprintf(stderr,
                "QuixoIsLegalPosition: GenericHashUnhashLabel error on tier "
                "%" PRITier " position %" PRIPos "\n",
                tier, position);
        return false;
    }

    int turn = GenericHashGetTurnLabel(tier, position);
    char opponent_piece = kPlayerPiece[OpponentsTurn(turn)];
    for (int i = 0; i < num_edge_slots; ++i) {
        int edge_index = edge_indices[i];
        char piece = board[edge_index];
        if (piece == opponent_piece) return true;
    }

    return false;
}

static int NumSymmetries(void) { return (board_rows == board_cols) ? 8 : 4; }

static Position QuixoDoSymmetry(Tier tier, const char *original_board, int turn,
                                int symmetry) {
    char symmetry_board[kBoardSizeMax];

    // Copy from the symmetry matrix.
    for (int i = 0; i < GetBoardSize(); ++i) {
        symmetry_board[i] = original_board[symmetry_matrix[symmetry][i]];
    }

    return GenericHashHashLabel(tier, symmetry_board, turn);
}

static Position QuixoGetCanonicalPosition(TierPosition tier_position) {
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) return kIllegalPosition;

    int turn = GenericHashGetTurnLabel(tier, position);
    Position canonical_position = position;
    for (int i = 0; i < NumSymmetries(); ++i) {
        Position new_position = QuixoDoSymmetry(tier, board, turn, i);
        if (new_position < canonical_position) {
            canonical_position = new_position;
        }
    }

    return canonical_position;
}

static int QuixoGetNumberOfCanonicalChildPositions(TierPosition tier_position) {
    int count = 0;
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) return -1;

    int turn = GenericHashGetTurnLabel(tier, position);
    int next_turn = OpponentsTurn(turn);
    char piece_to_move = kPlayerPiece[turn];
    TierPositionHashSet deduplication_set;
    TierPositionHashSetInit(&deduplication_set, 0.5);

    // Find all blank or friendly pieces on 4 edges and 4 corners of the board.
    for (int i = 0; i < num_edge_slots; ++i) {
        int edge_index = edge_indices[i];
        char piece = board[edge_index];
        // Skip opponent pieces, which cannot be moved.
        if (piece != kBlank && piece != piece_to_move) continue;

        for (int j = 0; j < edge_move_count[i]; ++j) {
            int src = edge_index, dest = edge_move_dests[i][j];
            MoveAndShiftPieces(board, src, dest, piece_to_move);  // Do move.
            bool flip = (piece == kBlank);
            TierPosition child = {.tier = GetChildTier(tier, turn, flip)};
            child.position = GenericHashHashLabel(child.tier, board, next_turn);
            MoveAndShiftPieces(board, dest, src, piece);  // Undo move.
            child.position = QuixoGetCanonicalPosition(child);
            if (TierPositionHashSetContains(&deduplication_set, child)) {
                continue;  // Already included.
            }
            TierPositionHashSetAdd(&deduplication_set, child);
            ++count;
        }
    }
    TierPositionHashSetDestroy(&deduplication_set);

    return count;
}

static TierPositionArray QuixoGetCanonicalChildPositions(
    TierPosition tier_position) {
    //
    TierPositionArray children;
    TierPositionArrayInit(&children);
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) {
        children.size = -1;
        return children;
    }

    int turn = GenericHashGetTurnLabel(tier, position);
    int next_turn = OpponentsTurn(turn);
    char piece_to_move = kPlayerPiece[turn];
    TierPositionHashSet deduplication_set;
    TierPositionHashSetInit(&deduplication_set, 0.5);

    // Find all blank or friendly pieces on 4 edges and 4 corners of the board.
    for (int i = 0; i < num_edge_slots; ++i) {
        int edge_index = edge_indices[i];
        char piece = board[edge_index];
        // Skip opponent pieces, which cannot be moved.
        if (piece != kBlank && piece != piece_to_move) continue;

        for (int j = 0; j < edge_move_count[i]; ++j) {
            int src = edge_index, dest = edge_move_dests[i][j];
            MoveAndShiftPieces(board, src, dest, piece_to_move);  // Do move.
            bool flip = (piece == kBlank);
            TierPosition child = {.tier = GetChildTier(tier, turn, flip)};
            child.position = GenericHashHashLabel(child.tier, board, next_turn);
            MoveAndShiftPieces(board, dest, src, piece);  // Undo move.
            child.position = QuixoGetCanonicalPosition(child);
            if (TierPositionHashSetContains(&deduplication_set, child)) {
                continue;  // Already included.
            }
            TierPositionHashSetAdd(&deduplication_set, child);
            TierPositionArrayAppend(&children, child);
        }
    }
    TierPositionHashSetDestroy(&deduplication_set);

    return children;
}

static bool IsCorrectFlipping(Tier child, Tier parent, int child_turn) {
    // Not flipping is always allowed.
    if (child == parent) return true;

    int child_num_blanks, child_num_x, child_num_o;
    UnhashTier(child, &child_num_blanks, &child_num_x, &child_num_o);

    int parent_num_blanks, parent_num_x, parent_num_o;
    UnhashTier(parent, &parent_num_blanks, &parent_num_x, &parent_num_o);

    assert(child_num_blanks >= 0 && child_num_x >= 0 && child_num_o >= 0);
    assert(child_num_blanks + child_num_x + child_num_o == GetBoardSize());
    assert(parent_num_blanks >= 0 && parent_num_x >= 0 && parent_num_o >= 0);
    assert(parent_num_blanks + parent_num_x + parent_num_o == GetBoardSize());
    assert(child_num_blanks == parent_num_blanks - 1);

    if (child_num_x == parent_num_x + 1) {  // Flipping an X.
        return child_turn == 2;  // Must be O's turn at child position.
    }
    // Else, flipping an O.
    assert(child_num_o == parent_num_o + 1);
    return child_turn == 1;
}

static PositionArray QuixoGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier) {
    //
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    PositionArray parents;
    PositionArrayInit(&parents);

    int turn = GenericHashGetTurnLabel(tier, position);
    assert(turn == 1 || turn == 2);
    if (!IsCorrectFlipping(tier, parent_tier, turn)) return parents;

    int prior_turn = OpponentsTurn(turn);  // Player who made the last move.
    bool flipped = (tier != parent_tier);  // Tier must be different if flipped.
    char opponents_piece = kPlayerPiece[prior_turn];
    char piece_to_move = flipped ? kBlank : opponents_piece;

    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) {
        fprintf(
            stderr,
            "QuixoGetCanonicalParentPositions: GenericHashUnhashLabel error\n");
        return parents;
    }

    PositionHashSet deduplication_set;
    PositionHashSetInit(&deduplication_set, 0.5);
    for (int i = 0; i < num_edge_slots; ++i) {
        int dest = edge_indices[i];
        for (int j = 0; j < edge_move_count[i]; ++j) {
            int src = edge_move_dests[i][j];
            if (board[src] != opponents_piece) continue;

            // An opponent move from dest to src is possible. Hence we "undo"
            // that move here by moving the opponent's piece from src to dest.
            MoveAndShiftPieces(board, src, dest, piece_to_move);  // Undo move
            TierPosition parent = {.tier = parent_tier};
            parent.position =
                GenericHashHashLabel(parent_tier, board, prior_turn);
            MoveAndShiftPieces(board, dest, src, opponents_piece);  // Restore
            parent.position = QuixoGetCanonicalPosition(parent);
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

static TierArray QuixoGetChildTiers(Tier tier) {
    int num_blanks, num_x, num_o;
    UnhashTier(tier, &num_blanks, &num_x, &num_o);
    assert(num_blanks + num_x + num_o == GetBoardSize());
    assert(num_blanks >= 0 && num_x >= 0 && num_o >= 0);
    TierArray children;
    TierArrayInit(&children);
    if (num_blanks > 0) {
        Tier x_flipped = HashTier(num_blanks - 1, num_x + 1, num_o);
        Tier o_flipped = HashTier(num_blanks - 1, num_x, num_o + 1);
        TierArrayAppend(&children, x_flipped);
        TierArrayAppend(&children, o_flipped);
    }  // no children for tiers with num_blanks == 0

    return children;
}

static int QuixoGetTierName(Tier tier, char name[static kDbFileNameLengthMax + 1]) {
    int num_blanks, num_x, num_o;
    UnhashTier(tier, &num_blanks, &num_x, &num_o);
    if (num_blanks < 0 || num_x < 0 || num_o < 0) return kIllegalGameTierError;
    if (num_blanks + num_x + num_o != GetBoardSize()) {
        return kIllegalGameTierError;
    }

    int actual_length = snprintf(name, kDbFileNameLengthMax + 1,
                                 "%dBlank_%dX_%dO", num_blanks, num_x, num_o);
    if (actual_length >= kDbFileNameLengthMax + 1) {
        fprintf(stderr, "QuixoGetTierName: (BUG) tier db filename overflow\n");
        return kMemoryOverflowError;
    }

    return kNoError;
}

/// Gameplay

static int QuixoTierPositionToString(TierPosition tier_position, char *buffer) {
    char board[kBoardSizeMax] = {0};
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    if (!success) return kRuntimeError;

    int offset = 0;
    for (int r = 0; r < board_rows; ++r) {
        if (r == (board_rows - 1) / 2) {
            offset += sprintf(buffer + offset, "LEGEND: ");
        } else {
            offset += sprintf(buffer + offset, "        ");
        }

        for (int c = 0; c < board_cols; ++c) {
            int index = r * board_cols + c + 1;
            if (c == 0) {
                offset += sprintf(buffer + offset, "(%2d", index);
            } else {
                offset += sprintf(buffer + offset, " %2d", index);
            }
        }
        offset += sprintf(buffer + offset, ")");

        if (r == (board_rows - 1) / 2) {
            offset += sprintf(buffer + offset, "    BOARD: : ");
        } else {
            offset += sprintf(buffer + offset, "           : ");
        }

        for (int c = 0; c < board_cols; ++c) {
            int index = r * board_cols + c;
            offset += sprintf(buffer + offset, "%c ", board[index]);
        }
        offset += sprintf(buffer + offset, "\n");
    }
    return kNoError;
}

static int QuixoMoveToString(Move move, char *buffer) {
    int src, dest;
    UnpackMove(move, &src, &dest);

    int actual_length = snprintf(
        buffer, QuixoGameplayApiCommon.move_string_length_max + 1, "%d %d",
        src + 1, dest + 1);  // add 1 to src and dest for user representation
    if (actual_length >= QuixoGameplayApiCommon.move_string_length_max + 1) {
        fprintf(stderr,
                "QuixoMoveToString: (BUG) not enough space was allocated "
                "to buffer. Please increase move_string_length_max.\n");
        return kMemoryOverflowError;
    }

    return kNoError;
}

static bool QuixoIsValidMoveString(ReadOnlyString move_string) {
    // Strings of format "source destination". Ex: "6 10"
    // Only "1" - "<board_size>" are valid move strings for both source and
    // dest.

    if (move_string == NULL) return false;
    if (strlen(move_string) < 3 || strlen(move_string) > 5) return false;

    // Make a copy of move_string bc strtok mutates the original move_string
    char copy_string[6];
    strcpy(copy_string, move_string);

    char *token = strtok(copy_string, " ");
    if (token == NULL) return false;
    int src = atoi(token);

    token = strtok(NULL, " ");
    if (token == NULL) return false;
    int dest = atoi(token);

    int board_size = GetBoardSize();
    if (src < 1) return false;
    if (src > board_size) return false;
    if (dest < 1) return false;
    if (dest > board_size) return false;

    return true;
}

static Move QuixoStringToMove(ReadOnlyString move_string) {
    // Strings of format "source destination". Ex: "6 10"
    char copy_string[6];
    strcpy(copy_string, move_string);
    int src = atoi(strtok(copy_string, " "));
    int dest = atoi(strtok(NULL, " "));

    return ConstructMove(src - 1, dest - 1);
}

// Uwapi

static bool QuixoIsLegalFormalPosition(ReadOnlyString formal_position) {
    // Check if FORMAL_POSITION is of the correct format.
    if (formal_position[0] != '1' && formal_position[0] != '2') return false;
    if (formal_position[1] != '_') return false;
    if (strlen(formal_position) != (size_t)(2 + GetBoardSize())) return false;

    // Check if the board corresponds to a valid tier.
    ConstantReadOnlyString board = formal_position + 2;
    int turn = formal_position[0] - '0';
    int num_blanks = 0, num_x = 0, num_o = 0;
    for (int i = 0; i < GetBoardSize(); ++i) {
        if (board[i] != kBlank && board[i] != kX && board[i] != kO) {
            return false;
        }
        num_blanks += (board[i] == kBlank);
        num_x += (board[i] == kX);
        num_o += (board[i] == kO);
    }
    if (!IsValidPieceConfig(num_blanks, num_x, num_o)) return false;

    // Check if the board is a valid position.
    TierPosition tier_position = {.tier = HashTier(num_blanks, num_x, num_o)};
    tier_position.position =
        GenericHashHashLabel(tier_position.tier, board, turn);
    if (!QuixoIsLegalPosition(tier_position)) return false;

    return true;
}

static TierPosition QuixoFormalPositionToTierPosition(
    ReadOnlyString formal_position) {
    //
    ConstantReadOnlyString board = formal_position + 2;
    int turn = formal_position[0] - '0';
    int num_blanks = 0, num_x = 0, num_o = 0;
    for (int i = 0; i < GetBoardSize(); ++i) {
        num_blanks += (board[i] == kBlank);
        num_x += (board[i] == kX);
        num_o += (board[i] == kO);
    }

    TierPosition ret = {.tier = HashTier(num_blanks, num_x, num_o)};
    ret.position = GenericHashHashLabel(ret.tier, board, turn);

    return ret;
}

static CString QuixoTierPositionToFormalPosition(TierPosition tier_position) {
    CString ret;
    CStringInitEmpty(&ret);
    CStringResize(&ret, 2 + GetBoardSize(), '\0');

    // Set turn.
    int turn =
        GenericHashGetTurnLabel(tier_position.tier, tier_position.position);
    ret.str[0] = '0' + turn;
    ret.str[1] = '_';

    // Set board.
    GenericHashUnhashLabel(tier_position.tier, tier_position.position,
                           ret.str + 2);

    return ret;
}

static CString QuixoTierPositionToAutoGuiPosition(TierPosition tier_position) {
    // AutoGUI currently does not support mapping '-' (the blank piece) to
    // images. Must use a different character here.
    CString ret = QuixoTierPositionToFormalPosition(tier_position);
    for (int64_t i = 0; i < ret.length; ++i) {
        if (ret.str[i] == '-') ret.str[i] = 'B';
    }

    return ret;
}

static CString QuixoMoveToFormalMove(TierPosition tier_position, Move move) {
    (void)tier_position;  // Unused;
    int src, dest;
    UnpackMove(move, &src, &dest);
    char buf[2 + 2 * kInt32Base10StringLengthMax];
    sprintf(buf, "%d %d", src, dest);
    CString ret;
    CStringInitCopyCharArray(&ret, buf);

    return ret;
}

static CString QuixoMoveToAutoGuiMove(TierPosition tier_position, Move move) {
    // Format: M_<src>_<dest>_x, where src is the center of the source tile and
    // dest is an invisible center inside the source tile in the move direction.
    // The first board_size centers (indexed from 0 - board_size - 1) correspond
    // to the centers for the tiles. The next board_size centers (board_size - 2
    // * board_size - 1) correspond to the destinations for all upward arrows.
    // Then follow left, downward, and right arrows. The sound effect character
    // is hard coded as an 'x'.
    (void)tier_position;  // Unused;
    int src, dest, dest_center, board_size = GetBoardSize();
    UnpackMove(move, &src, &dest);
    if (Abs(dest - src) < board_cols) {  // same row
        if (dest < src) {                // moving left
            dest_center = src + 4 * board_size;
        } else {  // moving right
            dest_center = src + 2 * board_size;
        }
    } else {               // same column
        if (dest < src) {  // moving up
            dest_center = src + board_size;
        } else {  // moving down
            dest_center = src + 3 * board_size;
        }
    }

    char buf[16];
    sprintf(buf, "M_%d_%d_x", src, dest_center);
    CString ret;
    CStringInitCopyCharArray(&ret, buf);

    return ret;
}

static void UpdateEdgeSlots(void) {
    num_edge_slots = 0;
    // Loop through all indices and append the indices of the ones on an edge.
    for (int i = 0; i < GetBoardSize(); ++i) {
        int row, col;
        BoardIndexToRowCol(i, &row, &col);
        if (row == 0 || col == 0 || row == board_rows - 1 ||
            col == board_cols - 1) {
            edge_indices[num_edge_slots] = i;
            edge_move_count[num_edge_slots] =
                GetMoveDestinations(edge_move_dests[num_edge_slots], i);
            ++num_edge_slots;
        }
    }
}

// Assumes DESTS has enough space.
static int GetMoveDestinations(int *dests, int src) {
    int row, col;
    BoardIndexToRowCol(src, &row, &col);
    int num_dests = 0;

    // Can move left if not at the left-most column.
    if (col > 0) {
        dests[num_dests++] = BoardRowColToIndex(row, 0);
    }

    // Can move right if not at the right-most column.
    if (col < board_cols - 1) {
        dests[num_dests++] = BoardRowColToIndex(row, board_cols - 1);
    }

    // Can move up if not at the top-most row.
    if (row > 0) {
        dests[num_dests++] = BoardRowColToIndex(0, col);
    }

    // Can move down if not at the bottom-most row.
    if (row < board_rows - 1) {
        dests[num_dests++] = BoardRowColToIndex(board_rows - 1, col);
    }

    assert(num_dests == 3 || num_dests == 2);
    return num_dests;
}

static int GetBoardSize(void) { return board_rows * board_cols; }

static Tier QuixoSetInitialTier(void) {
    int num_blanks = GetBoardSize(), num_x = 0, num_o = 0;
    initial_tier = HashTier(num_blanks, num_x, num_o);
    return initial_tier;
}

// Assumes Generic Hash has been initialized.
static Position QuixoSetInitialPosition(void) {
    char board[kBoardSizeMax];
    memset(board, kBlank, GetBoardSize());
    initial_position = GenericHashHashLabel(QuixoGetInitialTier(), board, 1);
    return initial_position;
}

static Tier HashTier(int num_blanks, int num_x, int num_o) {
    // Each number may take values from 0 to board_size. Hence there are
    // board_size + 1 possible values.
    int base = GetBoardSize() + 1;
    return num_o * base * base + num_x * base + num_blanks;
}

static void UnhashTier(Tier tier, int *num_blanks, int *num_x, int *num_o) {
    // Each number may take values from 0 to board_size. Hence there are
    // board_size + 1 possible values.
    int base = GetBoardSize() + 1;
    *num_blanks = tier % base;
    tier /= base;
    *num_x = tier % base;
    tier /= base;
    *num_o = tier;
}

/* Rotates the SRC board 90 degrees clockwise and stores resulting board in
 * DEST, assuming both are of dimensions board_rows by board_cols. */
static void Rotate90(int *dest, int *src) {
    for (int r = 0; r < board_rows; r++) {
        for (int c = 0; c < board_cols; c++) {
            int new_r = c;
            int new_c = board_rows - r - 1;
            dest[new_r * board_rows + new_c] = src[r * board_cols + c];
        }
    }
}

/* Reflects the SRC board across the middle column and stores resulting board in
 * DEST, assuming both are of dimensions board_rows by board_cols. */
static void Mirror(int *dest, int *src) {
    for (int r = 0; r < board_rows; r++) {
        for (int c = 0; c < board_cols; c++) {
            int new_r = r;
            int new_c = board_cols - c - 1;
            dest[new_r * board_cols + new_c] = src[r * board_cols + c];
        }
    }
}

static void QuixoInitSymmMatrix(void) {
    int board_size = GetBoardSize();
    assert(board_size <= kBoardSizeMax);

    // Manually generate the original board index layout.
    for (int i = 0; i < board_size; ++i) {
        symmetry_matrix[0][i] = i;
    }

    // Fill in the symmetric index layouts
    if (board_rows == board_cols) {  // Square has 8 symmetries
        Rotate90(symmetry_matrix[1], symmetry_matrix[0]);  // Rotate 90
        Rotate90(symmetry_matrix[2], symmetry_matrix[1]);  // Rotate 180
        Rotate90(symmetry_matrix[3], symmetry_matrix[2]);  // Rotate 270
        Mirror(symmetry_matrix[4], symmetry_matrix[0]);    // Mirror
        Rotate90(symmetry_matrix[5], symmetry_matrix[4]);  // Rotate mirror 90
        Rotate90(symmetry_matrix[6], symmetry_matrix[5]);  // Rotate mirror 180
        Rotate90(symmetry_matrix[7], symmetry_matrix[6]);  // Rotate mirror 270
    } else {  // Rectangle has 4 symmetries
        int tmp[kBoardSizeMax];
        Rotate90(tmp, symmetry_matrix[0]);
        Rotate90(symmetry_matrix[1], tmp);               // Rotate 180
        Mirror(symmetry_matrix[2], symmetry_matrix[0]);  // Mirror
        Rotate90(tmp, symmetry_matrix[2]);
        Rotate90(symmetry_matrix[3], tmp);  // Rotate mirror 180
    }
}

/**
 * @brief Returns 2 if TURN is 1, or 1 if TURN is 2. Assumes TURN is either 1
 * or 2.
 */
static int OpponentsTurn(int turn) { return (!(turn - 1)) + 1; }

static Move ConstructMove(int src, int dest) {
    return src * GetBoardSize() + dest;
}

static void UnpackMove(Move move, int *src, int *dest) {
    int board_size = GetBoardSize();
    *dest = move % board_size;
    *src = move / board_size;
}

static int BoardRowColToIndex(int row, int col) {
    return row * board_cols + col;
}

static void BoardIndexToRowCol(int index, int *row, int *col) {
    *row = index / board_cols;
    *col = index % board_cols;
}

/** @brief Returns whether there is a k_in_a_row of PIECEs on BOARD. */
static bool HasKInARow(const char *board, char piece) {
    for (int i = 0; i < board_rows; ++i) {
        for (int j = 0; j < board_cols; ++j) {
            // For each slot in BOARD, check 4 directions: right, down-right,
            // down, down-left.
            if (CheckDirection(board, piece, i, j, 0, 1)) return true;
            if (CheckDirection(board, piece, i, j, 1, 1)) return true;
            if (CheckDirection(board, piece, i, j, 1, 0)) return true;
            if (CheckDirection(board, piece, i, j, 1, -1)) return true;
        }
    }

    return false;
}

static bool CheckDirection(const char *board, char piece, int i, int j,
                           int dir_i, int dir_j) {
    int count = 0;
    while (i >= 0 && i < board_rows && j >= 0 && j < board_cols) {
        if (board[i * board_cols + j] == piece) {
            if (++count == k_in_a_row) return true;
        } else {
            return false;
        }
        i += dir_i;
        j += dir_j;
    }

    return false;
}

#include "games/quixo/quixo.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // NULL
#include <stdio.h>     // fprintf, stderr
#include <string.h>    // memset

#include "core/generic_hash/generic_hash.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

static int QuixoInit(void *aux);
static int InitGenericHash(void);
static bool IsValidPieceConfig(int num_blanks, int num_x, int num_o);
static Tier HashTier(int num_blanks, int num_x, int num_o);
static int QuixoFinalize(void);

static Tier QuixoGetInitialTier(void);
static Position QuixoGetInitialPosition(void);

static int64_t QuixoGetTierSize(Tier tier);
static MoveArray QuixoGenerateMoves(TierPosition tier_position);
static Value QuixoPrimitive(TierPosition tier_position);
static TierPosition QuixoDoMove(TierPosition tier_position, Move move);
static bool QuixoIsLegalPosition(TierPosition tier_position);
static Position QuixoGetCanonicalPosition(TierPosition tier_position);
static PositionArray QuixoGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier);
static TierArray QuixoGetChildTiers(Tier tier);

static const TierSolverApi kQuixoSolverApi = {
    .GetInitialTier = &QuixoGetInitialTier,
    .GetInitialPosition = &QuixoGetInitialPosition,

    .GetTierSize = &QuixoGetTierSize,
    .GenerateMoves = &QuixoGenerateMoves,
    .Primitive = &QuixoPrimitive,
    .DoMove = &QuixoDoMove,
    .IsLegalPosition = &QuixoIsLegalPosition,
    .GetCanonicalPosition = &QuixoGetCanonicalPosition,
    .GetCanonicalChildPositions = NULL,
    .GetCanonicalParentPositions = &QuixoGetCanonicalParentPositions,
    .GetPositionInSymmetricTier = NULL,
    .GetChildTiers = &QuixoGetChildTiers,
    .GetParentTiers = NULL,
    .GetCanonicalTier = NULL,
};

// -----------------------------------------------------------------------------

const Game kQuixo = {
    .name = "quixo",
    .formal_name = "Quixo",
    .solver = &kTierSolver,
    .solver_api = (const void *)&kQuixoSolverApi,
    .gameplay_api = (const GameplayApi *)NULL,  // TODO
    .uwapi = NULL,                              // TODO

    .Init = &QuixoInit,
    .Finalize = &QuixoFinalize,

    .GetCurrentVariant = NULL,
    .SetVariantOption = NULL,
};

// -----------------------------------------------------------------------------

enum QuixoConstants {
    kBoardRowsMax = 6,
    kBoardColsMax = 6,
    kBoardSizeMax = kBoardRowsMax * kBoardColsMax,
};

static int board_rows;  // Number of rows on the board (default 5).
static int board_cols;  // Number of columns on the board (default 5).
static int k_in_a_row;  // Number of pieces in a row a player needs to win.

static int GetBoardSize(void);
static Tier HashTier(int num_blanks, int num_x, int num_o);

static int QuixoInit(void *aux) {
    (void)aux;                    // Unused.
    board_rows = board_cols = 5;  // TODO: add variants.
    return InitGenericHash();
}

static int InitGenericHash(void) {
    GenericHashReinitialize();

    // Default to two players. Some tiers can only be one of the two
    // players' turns. Those tiers are dealt with in the for loop.
    static const int kTwoPlayerInitializer = 0;
    int board_size = GetBoardSize();
    // These values are just place holders. The actual values are generated
    // in the for loop below.
    int pieces_init_array[10] = {'-', 0, 0, 'X', 0, 0, 'O', 0, 0, -1};

    bool success;
    for (int num_blanks = 0; num_blanks <= board_size; ++num_blanks) {
        for (int num_x = 0; num_x <= board_size; ++num_x) {
            for (int num_o = 0; num_o <= board_size; ++num_o) {
                if (!IsValidPieceConfig(num_blanks, num_x, num_o)) continue;
                Tier tier = HashTier(num_blanks, num_x, num_o);
                pieces_init_array[1] = pieces_init_array[2] = num_blanks;
                pieces_init_array[4] = pieces_init_array[5] = num_x;
                pieces_init_array[7] = pieces_init_array[8] = num_o;

                int player = kTwoPlayerInitializer;
                if (num_blanks == board_size) {
                    player = 1;  // X always goes first.
                } else if (num_blanks == board_size - 1) {
                    player = 2;  // O always flips the second piece.
                }
                success = GenericHashAddContext(player, GetBoardSize(),
                                                pieces_init_array, NULL, tier);
                if (!success) {
                    fprintf(
                        stderr,
                        "InitGenericHash: failed to initialize generic hash "
                        "context for tier %" PRId64 ". Aborting...\n",
                        tier);  // TODO: merge and replace with PRITier.
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
    if (num_blanks <= board_size - 2) return num_x == 1 && num_o == 1;
    if (num_blanks == board_size - 1) return num_x == 1 && num_o == 0;

    return true;
}

static int QuixoFinalize(void) {
    GenericHashReinitialize();
    return kNoError;
}

static Tier QuixoGetInitialTier(void) {
    int num_blanks = GetBoardSize(), num_x = 0, num_o = 0;
    return HashTier(num_blanks, num_x, num_o);
}

static Position QuixoGetInitialPosition(void) {
    char board[kBoardSizeMax];
    memset(board, '-', GetBoardSize());
    return GenericHashHashLabel(QuixoGetInitialTier(), board, 1);
}

static int64_t QuixoGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static MoveArray QuixoGenerateMoves(TierPosition tier_position) {
    // TODO
    //
}

static bool CheckDirection(const char *board, char piece, int i, int j,
                           int dir_i, int dir_j) {
    int count = 0;
    while (i >= 0 && i < board_rows && j >= 0 && j < board_cols) {
        if (board[i * board_cols + j]) {
            if (++count == k_in_a_row) return true;
        } else {
            return false;
        }
        i += dir_i;
        j += dir_j;
    }

    return false;
}

/** @brief Returns whether there is a k_in_a_row of PIECEs in BOARD. */
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

static Value QuixoPrimitive(TierPosition tier_position) {
    char board[kBoardSizeMax];
    int turn = GenericHashGetTurn(tier_position.position);
    assert(turn == 1 || turn == 2);
    char my_piece = (turn == 1) ? 'X' : 'O';
    char opponent_piece = (turn == 1) ? 'O' : 'X';
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    assert(success);
    (void)success;

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

static TierPosition QuixoDoMove(TierPosition tier_position, Move move) {
    // TODO
}

static bool QuixoIsLegalPosition(TierPosition tier_position) {
    // TODO
}

static Position QuixoGetCanonicalPosition(TierPosition tier_position) {
    // TODO
}

static PositionArray QuixoGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier) {
    // TODO
}

static TierArray QuixoGetChildTiers(Tier tier) {
    // TODO
}

static int GetBoardSize(void) { return board_rows * board_cols; }

static Tier HashTier(int num_blanks, int num_x, int num_o) {
    int board_size = GetBoardSize();
    return num_o * board_size * board_size + num_x * board_size + num_blanks;
}

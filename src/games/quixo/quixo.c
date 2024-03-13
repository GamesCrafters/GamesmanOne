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
static Position QuixoGetPositionInSymmetricTier(TierPosition tier_position,
                                                Tier symmetric);
static TierArray QuixoGetChildTiers(Tier tier);
static Tier QuixoGetCanonicalTier(Tier tier);

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
    .GetPositionInSymmetricTier = &QuixoGetPositionInSymmetricTier,
    .GetChildTiers = &QuixoGetChildTiers,
    .GetParentTiers = NULL,
    .GetCanonicalTier = &QuixoGetCanonicalTier,
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
static int edge_indices[kBoardSizeMax];  // (calculated) Indices of edge slots.

// Helper functions for tier initialization.
static void UpdateEdgeSlots(void);
static int GetBoardSize(void);
static Tier QuixoSetInitialTier(void);
static Position QuixoSetInitialPosition(void);
static Tier HashTier(int num_blanks, int num_x, int num_o);
static void UnhashTier(Tier tier, int *num_blanks, int *num_x, int *num_o);

// Helper functions for QuixoGenerateMoves
static int OpponentsTurn(int turn);
static Move ConstructMove(int src, int dest);
static void UnpackMove(Move move, int *src, int *dest);
static int GetMoveDestinations(int *dests, int src);
static int BoardRowColToIndex(int row, int col);
static void BoardIndexToRowCol(int index, int *row, int *col);

// Helper functions for QuixoPrimitive
static bool HasKInARow(const char *board, char piece);
static bool CheckDirection(const char *board, char piece, int i, int j,
                           int dir_i, int dir_j);

static int QuixoInit(void *aux) {
    (void)aux;                    // Unused.
    board_rows = board_cols = 5;  // TODO: add variants.
    k_in_a_row = 5;
    UpdateEdgeSlots();
    int ret = InitGenericHash();
    if (ret != kNoError) return ret;

    QuixoSetInitialTier();
    QuixoSetInitialPosition();

    return kNoError;
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

        int dests[3];
        int num_dests = GetMoveDestinations(dests, edge_index);
        for (int j = 0; j < num_dests; ++j) {
            MoveArrayAppend(&moves, ConstructMove(edge_index, dests[j]));
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

// TODO for Robert: clean up this function.
static TierPosition QuixoDoMove(TierPosition tier_position, Move move) {
    TierPosition ret;
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) return (TierPosition){-1, -1};  // TODO: merge and replace.

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
    ret.tier = tier;
    if (board[src] == kBlank) {
        int num_blanks, num_x, num_o;
        UnhashTier(tier, &num_blanks, &num_x, &num_o);
        --num_blanks;
        num_x += (kPlayerPiece[turn] == kX);
        num_o += (kPlayerPiece[turn] == kO);
        ret.tier = HashTier(num_blanks, num_x, num_o);
    }

    // Move and shift pieces.
    int offset;
    if (Abs(dest - src) < board_cols) {  // Row move
        if (dest < src) {
            // Insert piece on the left. Slide everything 1 right.
            offset = -1;
        } else {
            // Insert piece on the right. Slide everything 1 left.
            offset = 1;
        }
    } else {  // Column move
        if (dest < src) {
            // Insert piece to the top. Slide column down
            offset = -board_cols;
        } else {
            // Insert piece to the bottom. Slide column up
            offset = board_cols;
        }
    }
    assert(Abs(src - dest) % Abs(offset) == 0);
    for (int i = src; i != dest; i += offset) {
        board[i] = board[i + offset];
    }
    board[dest] = piece_to_move;
    ret.position = GenericHashHashLabel(ret.tier, board, OpponentsTurn(turn));

    return ret;
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
        fprintf(stderr, "QuixoIsLegalPosition: GenericHashUnhashLabel error\n");
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

static Position QuixoGetCanonicalPosition(TierPosition tier_position) {
    // TODO
}

static PositionArray QuixoGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier) {
    // TODO
}

static Position QuixoGetPositionInSymmetricTier(TierPosition tier_position,
                                                Tier symmetric) {
    Tier this_tier = tier_position.tier;
    Position this_position = tier_position.position;
    assert(QuixoGetCanonicalTier(symmetric) ==
           QuixoGetCanonicalTier(this_tier));

    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(this_tier, this_position, board);
    if (!success) return -1;  // TODO: merge and replace with kIllegalPosition

    // Swap all 'X' and 'O' pieces on the board.
    for (int i = 0; i < GetBoardSize(); ++i) {
        if (board[i] == kX) {
            board[i] = kO;
        } else if (board[i] == kO) {
            board[i] = kX;
        }
    }

    int turn = GenericHashGetTurnLabel(this_tier, this_position);
    int opponents_turn = OpponentsTurn(turn);

    return GenericHashHashLabel(symmetric, board, opponents_turn);
}

static TierArray QuixoGetChildTiers(Tier tier) {
    int num_blanks, num_x, num_o;
    UnhashTier(tier, &num_blanks, &num_x, &num_o);
    int board_size = GetBoardSize();
    assert(num_blanks + num_x + num_o == board_size);
    assert(num_blanks >= 0 && num_x >= 0 && num_o >= 0);
    TierArray children;
    TierPositionArrayInit(&children);
    if (num_blanks == board_size) {
        Tier next = HashTier(num_blanks - 1, 1, 0);
        TierArrayAppend(&children, next);
    } else if (num_blanks == board_size - 1) {
        Tier next = HashTier(num_blanks - 1, 1, 1);
        TierArrayAppend(&children, next);
    } else {
        Tier x_flipped = HashTier(num_blanks - 1, num_x + 1, num_o);
        Tier o_flipped = HashTier(num_blanks - 1, num_x, num_o + 1);
        TierArrayAppend(&children, x_flipped);
        TierArrayAppend(&children, o_flipped);
    }

    return children;
}

static Tier QuixoGetCanonicalTier(Tier tier) {
    int num_blanks, num_x, num_o;
    UnhashTier(tier, &num_blanks, &num_x, &num_o);

    // Swap the number of Xs and Os.
    Tier symm = HashTier(num_blanks, num_o, num_x);

    // Return the smaller of the two.
    return (tier < symm) ? tier : symm;
}

static void UpdateEdgeSlots(void) {
    num_edge_slots = 0;
    // Loop through all indices and append the indices of the ones on an edge.
    for (int i = 0; i < GetBoardSize(); ++i) {
        int row, col;
        BoardIndexToRowCol(i, &row, &col);
        if (row == 0 || col == 0 || row == board_rows - 1 ||
            col == board_cols - 1) {
            edge_indices[num_edge_slots++] = i;
        }
    }
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
    int board_size = GetBoardSize();
    return num_o * board_size * board_size + num_x * board_size + num_blanks;
}

static void UnhashTier(Tier tier, int *num_blanks, int *num_x, int *num_o) {
    int board_size = GetBoardSize();
    *num_blanks = tier % board_size;
    tier /= board_size;
    *num_x = tier % board_size;
    tier /= board_size;
    *num_o = tier;
}

/**
 * @brief Returns 2 if TURN is 1, or 1 if TURN is 2. Assumes TURN is either 1
 * or 2.
 */
static int OpponentsTurn(int turn) { return !(turn - 1) + 1; }

static Move ConstructMove(int src, int dest) {
    return src * GetBoardSize() + dest;
}

static void UnpackMove(Move move, int *src, int *dest) {
    int board_size = GetBoardSize();
    *dest = move % board_size;
    *src = move / board_size;
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

static int BoardRowColToIndex(int row, int col) {
    return row * board_cols + col;
}

static void BoardIndexToRowCol(int index, int *row, int *col) {
    row = index / board_cols;
    col = index % board_cols;
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

#include "games/mtttier/mtttier.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t
#include <stdio.h>     // fprintf, stderr

#include "core/gamesman.h"  // tier_solver
#include "core/gamesman_types.h"
#include "core/generic_hash/generic_hash.h"
#include "core/misc.h"  // Gamesman types utilities

// API Functions

static int64_t MtttierGetTierSize(Tier tier);
static MoveArray MtttierGenerateMoves(Tier tier, Position position);
static Value MtttierPrimitive(Tier tier, Position position);
static TierPosition MtttierDoMove(Tier tier, Position position, Move move);
static bool MtttierIsLegalPosition(Tier tier, Position position);
static Position MtttierGetCanonicalPosition(Tier tier, Position position);
static PositionArray MtttierGetCanonicalParentPositions(Tier tier,
                                                        Position position,
                                                        Tier parent_tier);
static TierArray MtttierGetChildTiers(Tier tier);
static TierArray MtttierGetParentTiers(Tier tier);

// Helper Types and Global Variables

static const int kNumRowsToCheck = 8;
static const int kRowsToCheck[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8},
                                       {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
                                       {0, 4, 8}, {2, 4, 6}};
static const int kNumSymmetries = 8;
static const int kSymmetryMatrix[8][9] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8}, {2, 5, 8, 1, 4, 7, 0, 3, 6},
    {8, 7, 6, 5, 4, 3, 2, 1, 0}, {6, 3, 0, 7, 4, 1, 8, 5, 2},
    {2, 1, 0, 5, 4, 3, 8, 7, 6}, {0, 3, 6, 1, 4, 7, 2, 5, 8},
    {6, 7, 8, 3, 4, 5, 0, 1, 2}, {8, 5, 2, 7, 4, 1, 6, 3, 0}};

// Helper Functions

static bool InitGenericHash(void);
static Position GetGlobalInitialPosition(void);
static char ThreeInARow(const char *board, const int *indices);
static bool AllFilledIn(const char *board);
static void CountPieces(const char *board, int *xcount, int *ocount);
static char WhoseTurn(const char *board);
static Position DoSymmetry(Tier tier, Position position, int symmetry);

// ----------------------------------------------------------------------------
bool MtttierInit(void) {
    if (!InitGenericHash()) return false;
    global_num_positions = kTierGamesmanGlobalNumberOfPositions;
    global_initial_tier = 0;
    global_initial_position = GetGlobalInitialPosition();
    tier_solver.GetTierSize = &MtttierGetTierSize;
    tier_solver.GenerateMoves = &MtttierGenerateMoves;
    tier_solver.Primitive = &MtttierPrimitive;
    tier_solver.DoMove = &MtttierDoMove;
    tier_solver.IsLegalPosition = &MtttierIsLegalPosition;
    tier_solver.GetCanonicalPosition = &MtttierGetCanonicalPosition;
    // tier_solver.GetCanonicalParentPositions =
    //     &MtttierGetCanonicalParentPositions;
    tier_solver.GetChildTiers = &MtttierGetChildTiers;
    tier_solver.GetParentTiers = &MtttierGetParentTiers;
    return true;
}
// ----------------------------------------------------------------------------

static int64_t MtttierGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static MoveArray MtttierGenerateMoves(Tier tier, Position position) {
    MoveArray moves;
    MoveArrayInit(&moves);

    char board[9] = {0};
    GenericHashUnhashLabel(tier, position, board);
    for (Move i = 0; i < 9; ++i) {
        if (board[i] == '-') {
            MoveArrayAppend(&moves, i);
        }
    }
    return moves;
}

static Value MtttierPrimitive(Tier tier, Position position) {
    char board[9] = {0};
    GenericHashUnhashLabel(tier, position, board);

    for (int i = 0; i < kNumRowsToCheck; ++i) {
        if (ThreeInARow(board, kRowsToCheck[i]) > 0) return kLose;
    }
    if (AllFilledIn(board)) return kTie;
    return kUndecided;
}

static TierPosition MtttierDoMove(Tier tier, Position position, Move move) {
    char board[9] = {0};
    GenericHashUnhashLabel(tier, position, board);
    char turn = WhoseTurn(board);
    board[move] = turn;
    TierPosition ret;
    ret.tier = tier + 1;
    ret.position = GenericHashHashLabel(tier, board, 1);
    return ret;
}

static bool MtttierIsLegalPosition(Tier tier, Position position) {
    // A position is legal if and only if:
    // 1. xcount == ocount or xcount = ocount + 1 if no one is winning and
    // 2. xcount == ocount if O is winning and
    // 3. xcount == ocount + 1 if X is winning and
    // 4. only one player can be winning
    char board[9] = {0};
    GenericHashUnhashLabel(tier, position, board);

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

static Position MtttierGetCanonicalPosition(Tier tier, Position position) {
    Position canonical_position = position;
    Position new_position;

    for (int i = 0; i < kNumSymmetries; ++i) {
        new_position = DoSymmetry(tier, position, i);
        if (new_position < canonical_position) {
            // By GAMESMAN convention, the canonical position is the one with
            // the smallest hash value.
            canonical_position = new_position;
        }
    }

    return canonical_position;
}

static PositionArray MtttierGetCanonicalParentPositions(Tier tier,
                                                        Position position,
                                                        Tier parent_tier) {
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
            Position parent = GenericHashHashLabel(tier - 1, board, 1);
            // Add piece back to the board.
            board[i] = prev_turn;
            if (!MtttierIsLegalPosition(tier - 1, parent)) {
                continue;  // Illegal.
            }
            parent = MtttierGetCanonicalPosition(tier - 1, parent);
            if (PositionHashSetContains(&deduplication_set, parent)) {
                continue;  // Already included.
            }
            PositionHashSetAdd(&deduplication_set, parent);
            PositionArrayAppend(&parents, parent);
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

static TierArray MtttierGetParentTiers(Tier tier) {
    TierArray parents;
    TierArrayInit(&parents);
    if (tier > 0) TierArrayAppend(&parents, tier - 1);
    return parents;
}

// Helper functions implementation

static bool InitGenericHash(void) {
    int player = 1;  // No turn bit needed as we can infer the turn from board.
    int board_size = 9;
    int pieces_init_array[10] = {'-', 9, 9, 'O', 0, 0, 'X', 0, 0, -1};
    for (Tier tier = 0; tier <= 9; ++tier) {
        // Adjust piece_init_array
        pieces_init_array[1] = pieces_init_array[2] = 9 - tier;
        pieces_init_array[4] = pieces_init_array[5] = tier / 2;
        pieces_init_array[7] = pieces_init_array[8] = (tier + 1) / 2;
        bool success = GenericHashAddContext(player, board_size,
                                             pieces_init_array, NULL, tier);
        if (!success) {
            fprintf(stderr,
                    "MtttierInit: failed to initialize generic hash context "
                    "for tier %" PRId64 ". Aborting...\n",
                    tier);
            GenericHashReinitialize();
            return false;
        }
    }
    return true;
}

static Position GetGlobalInitialPosition(void) {
    char board[9];
    for (int i = 0; i < 9; ++i) {
        board[i] = '-';
    }
    return GenericHashHashLabel(0, board, 1);
}

static char ThreeInARow(const char *board, const int *indices) {
    if (board[indices[0]] != board[indices[1]]) return 0;
    if (board[indices[1]] != board[indices[2]]) return 0;
    if (board[indices[0]] == '-') return 0;
    return board[indices[0]];
}

static bool AllFilledIn(const char *board) {
    for (int i = 0; i < 9; ++i) {
        if (board[i] == '-') return false;
    }
    return true;
}

static void CountPieces(const char *board, int *xcount, int *ocount) {
    *xcount = *ocount = 0;
    for (int i = 0; i < 9; ++i) {
        *xcount += (board[i] == 'X');
        *ocount += (board[i] == 'O');
    }
}

static char WhoseTurn(const char *board) {
    int xcount, ocount;
    CountPieces(board, &xcount, &ocount);
    if (xcount == ocount) return 'X';  // In our TicTacToe, x always goes first.
    return 'O';
}

static Position DoSymmetry(Tier tier, Position position, int symmetry) {
    char board[9] = {0}, symmetry_board[9] = {0};
    GenericHashUnhashLabel(tier, position, board);

    // Copy from the symmetry matrix.
    for (int i = 0; i < 9; ++i) {
        symmetry_board[i] = board[kSymmetryMatrix[symmetry][i]];
    }

    return GenericHashHashLabel(tier, symmetry_board, 1);
}

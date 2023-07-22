#include "games/mttt/mttt.h"

#include <stdbool.h>
#include <stdio.h>

#include "core/gamesman.h"
#include "core/gamesman_types.h"
#include "core/misc.h"  // NotReached

// API Function

static MoveArray MtttGenerateMoves(Position position);
static Value MtttPrimitive(Position position);
static Position MtttDoMove(Position position, Move move);
static bool MtttIsLegalPosition(Position position);
static Position MtttGetCanonicalPosition(Position position);
static PositionArray MtttGetCanonicalParentPositions(Position position);

// Helper Types and Global Variables

typedef enum PossibleBoardPieces { kBlank, kO, kX } BlankOX;

// Powers of 3 - this is the way I encode the position, as an integer.
static int three_to_the[] = {1, 3, 9, 27, 81, 243, 729, 2187, 6561};

static const int kNumRowsToCheck = 8;
static const int kRowsToCheck[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8},
                                       {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
                                       {0, 4, 8}, {2, 4, 6}};

// 8 symmetries, each one is a reordering of the 9 slots on the board.
// This will be initialized in the MtttInitialize function.
static const int num_symmetries = 8;
static int symmetry_matrix[8][9];

// Proofs of correctness for the below arrays:
//
//   FLIP                 ROTATE
//
// 0 1 2    2 1 0       0 1 2    6 3 0    8 7 6    2 5 8
// 3 4 5 -> 5 4 3       3 4 5 -> 7 4 1 -> 5 4 3 -> 1 4 7
// 6 7 8    8 7 6       6 7 8    8 5 2    2 1 0    0 3 6

// This is the array used for flipping along the N-S axis.
static const int flip_new_position[] = {2, 1, 0, 5, 4, 3, 8, 7, 6};

// This is the array used for rotating 90 degrees clockwise.
static const int rotate_90_clockwise_new_position[] = {6, 3, 0, 7, 4,
                                                       1, 8, 5, 2};

// Helper Functions

static void InitSymmetryMatrix(void);
static Position DoSymmetry(Position position, int symmetry);

static Position Hash(BlankOX *board);
static void Unhash(Position position, BlankOX *board);
static int ThreeInARow(BlankOX *board, const int *indices);
static bool AllFilledIn(BlankOX *board);
static void CountPieces(BlankOX *board, int *xcount, int *ocount);
static BlankOX WhoseTurn(BlankOX *board);

//-----------------------------------------------------------------------------
void MtttInitialize(void) {
    global_num_positions = 19683;  // 3**9.
    global_initial_position = 0;

    regular_solver.GenerateMoves = &MtttGenerateMoves;
    regular_solver.Primitive = &MtttPrimitive;
    regular_solver.DoMove = &MtttDoMove;
    regular_solver.IsLegalPosition = &MtttIsLegalPosition;
    regular_solver.GetCanonicalPosition = &MtttGetCanonicalPosition;
    // regular_solver.GetCanonicalParentPositions =
    // &MtttGetCanonicalParentPositions;

    InitSymmetryMatrix();
}
//-----------------------------------------------------------------------------

// API Implementation

static MoveArray MtttGenerateMoves(Position position) {
    MoveArray moves;
    MoveArrayInit(&moves);

    BlankOX board[9] = {0};
    Unhash(position, board);
    for (Move i = 0; i < 9; ++i) {
        if (board[i] == kBlank) {
            MoveArrayAppend(&moves, i);
        }
    }
    return moves;
}

static Value MtttPrimitive(Position position) {
    BlankOX board[9] = {0};
    Unhash(position, board);

    for (int i = 0; i < kNumRowsToCheck; ++i) {
        if (ThreeInARow(board, kRowsToCheck[i]) > 0) return kLose;
    }
    if (AllFilledIn(board)) return kTie;
    return kUndecided;
}

static Position MtttDoMove(Position position, Move move) {
    BlankOX board[9] = {0};
    Unhash(position, board);
    return position + three_to_the[move] * (int)WhoseTurn(board);
}

static bool MtttIsLegalPosition(Position position) {
    // A position is legal if and only if:
    // 1. xcount == ocount or xcount = ocount + 1 if no one is winning and
    // 2. xcount == ocount if O is winning and
    // 3. xcount == ocount + 1 if X is winning and
    // 4. only one player can be winning
    BlankOX board[9] = {0};
    Unhash(position, board);

    int xcount, ocount;
    CountPieces(board, &xcount, &ocount);
    if (xcount != ocount && xcount != ocount + 1) return false;

    bool xwin = false, owin = false;
    for (int i = 0; i < kNumRowsToCheck; ++i) {
        int row_value = ThreeInARow(board, kRowsToCheck[i]);
        xwin |= (row_value == kX);
        owin |= (row_value == kO);
    }
    if (xwin && owin) return false;
    if (xwin && xcount != ocount + 1) return false;
    if (owin && xcount != ocount) return false;
    return true;
}

static Position MtttGetCanonicalPosition(Position position) {
    Position canonical_position = position;
    Position new_position;

    for (int i = 0; i < num_symmetries; ++i) {
        new_position = DoSymmetry(position, i);
        if (new_position < canonical_position) {
            // By GAMESMAN convention, the canonical position is the one with
            // the smallest hash value.
            canonical_position = new_position;
        }
    }

    return canonical_position;
}

static PositionArray MtttGetCanonicalParentPositions(Position position) {
    PositionHashSet deduplication_set;
    PositionHashSetInit(&deduplication_set, 0.5);

    PositionArray parents;
    PositionArrayInit(&parents);

    BlankOX board[9] = {0};
    Unhash(position, board);

    BlankOX prev_turn = WhoseTurn(board) == kX ? kO : kX;
    for (int i = 0; i < 9; ++i) {
        if (board[i] == prev_turn) {
            Position parent = position - (int)prev_turn * three_to_the[i];
            parent = MtttGetCanonicalPosition(parent);
            if (!MtttIsLegalPosition(parent)) continue;  // Illegal.
            if (PositionHashSetContains(&deduplication_set, parent))
                continue;  // Already included.
            PositionHashSetAdd(&deduplication_set, parent);
            PositionArrayAppend(&parents, parent);
        }
    }
    PositionHashSetDestroy(&deduplication_set);

    return parents;
}

// Helper functions implementation

static void InitSymmetryMatrix(void) {
    for (int i = 0; i < 9; ++i) {
        int temp = i;
        for (int j = 0; j < num_symmetries; ++j) {
            if (j == num_symmetries / 2) temp = flip_new_position[i];
            if (j < num_symmetries / 2) {
                temp = symmetry_matrix[j][i] =
                    rotate_90_clockwise_new_position[temp];
            } else {
                temp = symmetry_matrix[j][i] =
                    rotate_90_clockwise_new_position[temp];
            }
        }
    }
}

static Position DoSymmetry(Position position, int symmetry) {
    BlankOX board[9] = {0}, symmetry_board[9] = {0};

    Unhash(position, board);

    // Copy from the symmetry matrix.
    for (int i = 0; i < 9; ++i) {
        symmetry_board[i] = board[symmetry_matrix[symmetry][i]];
    }

    return Hash(symmetry_board);
}

static Position Hash(BlankOX *board) {
    Position ret = 0;
    for (int i = 8; i >= 0; --i) {
        ret = ret * 3 + (int)board[i];
    }
    return ret;
}

static void Unhash(Position position, BlankOX *board) {
    // The following algorithm assumes kBlank == 0, kO == 1, and kX == 2.
    for (int i = 0; i < 9; ++i) {
        board[i] = (BlankOX)(position % 3);
        position /= 3;
    }
}

static int ThreeInARow(BlankOX *board, const int *indices) {
    if (board[indices[0]] != board[indices[1]]) return 0;
    if (board[indices[1]] != board[indices[2]]) return 0;
    if (board[indices[0]] == kBlank) return 0;
    return (int)board[indices[0]];
}

static bool AllFilledIn(BlankOX *board) {
    for (int i = 0; i < 9; ++i) {
        if (board[i] == kBlank) return false;
    }
    return true;
}

static void CountPieces(BlankOX *board, int *xcount, int *ocount) {
    *xcount = *ocount = 0;
    for (int i = 0; i < 9; ++i) {
        *xcount += (board[i] == kX);
        *ocount += (board[i] == kO);
    }
}

static BlankOX WhoseTurn(BlankOX *board) {
    int xcount, ocount;
    CountPieces(board, &xcount, &ocount);
    if (xcount == ocount) return kX;  // In our TicTacToe, x always goes first.
    return kO;
}

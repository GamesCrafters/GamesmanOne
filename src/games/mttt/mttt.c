#include "games/mttt/mttt.h"

#include <stdbool.h>
#include <stdio.h>

#include "core/gamesman.h"
#include "core/gamesman_types.h"
#include "core/misc.h"  // NotReached

/* Mandatory API */

MoveArray MtttGenerateMoves(Position position);
Value MtttPrimitive(Position position);
Position MtttDoMove(Position position, Move move);
bool MtttIsLegalPosition(Position position);
int MtttGetNumberOfChildPositions(Position position);
PositionArray MtttGetParentPositions(Position position);

void MtttInitialize(void) {
    global_num_positions = 19683;  // 3**9.
    global_initial_position = 0;

    GenerateMoves = &MtttGenerateMoves;
    Primitive = &MtttPrimitive;
    DoMove = &MtttDoMove;
    GetParentPositions = &MtttGetParentPositions;
    IsLegalPosition = &MtttIsLegalPosition;
    GetNumberOfChildPositions = &MtttGetNumberOfChildPositions;
    GetParentPositions = &MtttGetParentPositions;
}

/* API Implementation. */

typedef enum PossibleBoardPieces { kBlank, kO, kX } BlankOX;

// Powers of 3 - this is the way I encode the position, as an integer.
static int three_to_the[] = {1, 3, 9, 27, 81, 243, 729, 2187, 6561};

static const int kNumRowsToCheck = 8;
static const int kRowsToCheck[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8},
                                       {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
                                       {0, 4, 8}, {2, 4, 6}};

static void Unhash(Position position, BlankOX *board) {
    for (int i = 8; i >= 0; --i) {
        // The following algorithm assumes kX > kO > kBlank.
        if (position >= (Position)(kX * three_to_the[i])) {
            board[i] = kX;
            position -= kX * three_to_the[i];
        } else if (position >= (Position)(kO * three_to_the[i])) {
            board[i] = kO;
            position -= kO * three_to_the[i];
        } else if (position >= (Position)(kBlank * three_to_the[i])) {
            board[i] = kBlank;
            position -= kBlank * three_to_the[i];
        } else {
            NotReached(
                "mttt.c::Unhash: unable to place the piece with the smallest "
                "index.");
        }
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

MoveArray MtttGenerateMoves(Position position) {
    MoveArray moves;
    MoveArrayInit(&moves);

    BlankOX board[9];
    Unhash(position, board);
    for (Move i = 0; i < 9; ++i) {
        if (board[i] == kBlank) {
            MoveArrayAppend(&moves, i);
        }
    }
    return moves;
}

Value MtttPrimitive(Position position) {
    BlankOX board[9];
    Unhash(position, board);

    for (int i = 0; i < kNumRowsToCheck; ++i) {
        if (ThreeInARow(board, kRowsToCheck[i]) > 0) return kLose;
    }
    if (AllFilledIn(board)) return kTie;
    return kUndecided;
}

Position MtttDoMove(Position position, Move move) {
    BlankOX board[9];
    Unhash(position, board);
    return position + three_to_the[move] * (int)WhoseTurn(board);
}

bool MtttIsLegalPosition(Position position) {
    // A position is legal if and only if:
    // 1. xcount == ocount or xcount = ocount + 1 if no one is winning and
    // 2. xcount == ocount if O is winning and
    // 3. xcount == ocount + 1 if X is winning and
    // 4. only one player can be winning
    BlankOX board[9];
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

int MtttGetNumberOfChildPositions(Position position) {
    int num_blank = 0;
    for (int i = 0; i < 9; ++i) {
        num_blank += (position % 3 == 0);
        position /= 3;
    }
    return num_blank;
}

PositionArray MtttGetParentPositions(Position position) {
    PositionArray parents;
    PositionArrayInit(&parents);

    BlankOX board[9];
    Unhash(position, board);
    BlankOX prev_turn = WhoseTurn(board) == kX ? kO : kX;
    for (int i = 0; i < 9; ++i) {
        if (board[i] == prev_turn) {
            Position parent = position - (int)prev_turn * three_to_the[i];
            if (MtttIsLegalPosition(parent))
                PositionArrayAppend(&parents, parent);
        }
    }
    return parents;
}

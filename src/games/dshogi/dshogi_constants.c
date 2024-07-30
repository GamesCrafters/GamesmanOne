#include "games/dshogi/dshogi_constants.h"

#include <stdbool.h>  // bool
#include <stdint.h>   // int8_t

// Whether the following global variables have been initialized.
static bool initialized;
int8_t kPieceTypeToIndex[128];  // Initialized by InitPieceTypeToIndex
const char kIndexToPieceType[3] = {'G', 'E', 'C'};
int8_t kPieceToIndex[128];           // Initialized by InitPieceToIndex.
int8_t kMoveMatrix[10][12][8];       // Initialized by InitMoveMatrix.
int8_t kMoveMatrixNumMoves[10][12];  // Initialized by InitMoveMatrix.
bool kPromoteMatrix[10][12];         // Initialized by InitPromoteMatrix.
const int kNumSymmetries = 2;
const int8_t kSymmetryMatrix[2][12] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    {2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9},
};

static const bool kPieceMoveRuleMatrix[10][3][3] = {
    // Forest lion (L)
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
    },
    // Sky lion (l)
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
    },
    // Forest giraffe (G)
    {
        {0, 1, 0},
        {1, 0, 1},
        {0, 1, 0},
    },
    // Sky giraffe (g)
    {
        {0, 1, 0},
        {1, 0, 1},
        {0, 1, 0},
    },
    // Forest elephant (E)
    {
        {1, 0, 1},
        {0, 0, 0},
        {1, 0, 1},
    },
    // Sky elephant (e)
    {
        {1, 0, 1},
        {0, 0, 0},
        {1, 0, 1},
    },
    // Forest hen (H)
    {
        {1, 1, 1},
        {1, 0, 1},
        {0, 1, 0},
    },
    // Sky hen (h)
    {
        {0, 1, 0},
        {1, 0, 1},
        {1, 1, 1},
    },
    // Forest chick (C)
    {
        {0, 1, 0},
        {0, 0, 0},
        {0, 0, 0},
    },
    // Sky chick (c)
    {
        {0, 0, 0},
        {0, 0, 0},
        {0, 1, 0},
    },
};

static void InitPieceTypeToIndex(void) {
    for (int i = 0; i < 128; ++i) {
        kPieceTypeToIndex[i] = 3;
    }
    kPieceTypeToIndex[(int)'G'] = kPieceTypeToIndex[(int)'g'] = 0;
    kPieceTypeToIndex[(int)'E'] = kPieceTypeToIndex[(int)'e'] = 1;
    kPieceTypeToIndex[(int)'H'] = kPieceTypeToIndex[(int)'h'] = 2;
    kPieceTypeToIndex[(int)'C'] = kPieceTypeToIndex[(int)'c'] = 2;
}

static void InitPieceToIndex(void) {
    kPieceToIndex[(int)'L'] = 0;
    kPieceToIndex[(int)'l'] = 1;
    kPieceToIndex[(int)'G'] = 2;
    kPieceToIndex[(int)'g'] = 3;
    kPieceToIndex[(int)'E'] = 4;
    kPieceToIndex[(int)'e'] = 5;
    kPieceToIndex[(int)'H'] = 6;
    kPieceToIndex[(int)'h'] = 7;
    kPieceToIndex[(int)'C'] = 8;
    kPieceToIndex[(int)'c'] = 9;
}

static bool OnBoard(int i, int j) {
    if (i < 0) return false;
    if (j < 0) return false;
    if (i > 3) return false;
    if (j > 2) return false;

    return true;
}

static void InitMoveMatrix(void) {
    // Assumes kMoveMatrixNumMoves contain all zeros.
    for (int piece = 0; piece < 10; ++piece) {
        for (int slot = 0; slot < 12; ++slot) {
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    // Skip if this piece cannot move in that direction.
                    if (!kPieceMoveRuleMatrix[piece][i][j]) continue;

                    // Skip if this the destination is outside of the board.
                    int i_off = i - 1, j_off = j - 1;
                    if (!OnBoard(slot / 3 + i_off, slot % 3 + j_off)) continue;

                    int dest = slot + i_off * 3 + j_off;
                    kMoveMatrix[piece][slot]
                               [kMoveMatrixNumMoves[piece][slot]++] = dest;
                }
            }
        }
    }
}

static void InitPromoteMatrix(void) {
    // Assumes kPieceToIndex has already been initialized.
    for (int i = 0; i < 3; ++i) {
        kPromoteMatrix[kPieceToIndex[(int)'C']][i] = true;
    }
    for (int i = 9; i < 12; ++i) {
        kPromoteMatrix[kPieceToIndex[(int)'c']][i] = true;
    }
}

void DobutsuShogiInitGlobalVariables(void) {
    if (initialized) return;

    InitPieceTypeToIndex();
    InitPieceToIndex();
    InitMoveMatrix();
    InitPromoteMatrix();
    initialized = true;
}

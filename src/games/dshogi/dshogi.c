/**
 * @file dshogi.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of Dōbutsu shōgi, also known as "animal shogi," or
 * "Let's Catch the Lion!" Japanese: 「どうぶつしょうぎ」.
 * @details https://en.wikipedia.org/wiki/D%C5%8Dbutsu_sh%C5%8Dgi
 * @version 1.0.0
 * @date 2024-07-26
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

#include "games/dshogi/dshogi.h"

#include <assert.h>   // assert
#include <ctype.h>    // isupper, islower, tolower
#include <stdbool.h>  // bool, false
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // atoi, strtok
#include <string.h>   // strcpy, strlen

#include "core/constants.h"
#include "core/generic_hash/generic_hash.h"
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/types/gamesman_types.h"

// Global Constants

enum {
    /** @brief Size of the game board. */
    kBoardSize = 12,

    /**
     * @brief Size of the board string.
     * @details Note that this is not equal to the number of slots on the actual
     * game board. In addition to the 12 slots, there are 3 additional used as
     * counters for the unordered pieces held by the forest player.
     */
    kBoardStrSize = 12 + 3,

    /** @brief Number of positive signed characters in total. */
    kNumChars = 128,

    /** @brief Length of the formal position string. */
    kDobutsuShogiFormalPositionStrlen = 22,

    /** @brief Length of the AutoGui position string. */
    kDobutsuShogiAutoGuiPositionStrlen = 20,
};

static bool constants_initialized;

/** G -> 0, g -> 0, E -> 1, e -> 1, C -> 2, c -> 2, H -> 2, h -> 2, all others
 * are mapped to 3. Initialized by InitPieceTypeToIndex */
static int8_t kPieceTypeToIndex[kNumChars];

/** 0 -> G, 1 -> E, 2 -> C. */
static const char kIndexToPieceType[3] = {'G', 'E', 'C'};

/** L -> 0, l -> 1, G -> 2, g -> 3, E -> 4, e -> 5, H -> 6, h -> 7, C -> 8,
 *  c -> 9, - -> 10, all others are mapped to -1. Initialized by
 *  InitPieceToIndex. */
static int8_t kPieceToIndex[kNumChars];

/** kMoveMatrix[i][j][k] stores the k-th possible destination of piece of index
 *  i at the j-th slot of the board. The three dimensions correspond to piece
 *  index, board slot index, and available move index, respectively. Initialized
 *  by InitMoveMatrix. */
static int8_t kMoveMatrix[10][12][8];

/** kMoveMatrixNumMoves[i][j] is the size of kMoveMatrix[i][j].
 *  The two dimensions correspond to piece index and board slot index.
 *  Initialized by InitMoveMatrix. */
static int8_t kMoveMatrixNumMoves[10][12];

/** kPromoteMatrix[i][j] is true if and only if the piece of index i can be
 *  promoted once it moves to the j-th slot on the board. The two dimensions
 *  correspond to piece index and board slot index. Initialized by
 *  InitPromoteMatrix. */
static bool kPromoteMatrix[10][12];

/** Number of symmetries. For simplicity, side-swapping symmetry is not
 * implemented. */
static const int kNumSymmetries = 2;

/** The symmetry matrix. */
static const int8_t kSymmetryMatrix[2][12] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    {2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9},
};

/** Move rule for each type of piece. */
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
    for (int i = 0; i < kNumChars; ++i) {
        kPieceTypeToIndex[i] = 3;
    }
    kPieceTypeToIndex[(int)'G'] = kPieceTypeToIndex[(int)'g'] = 0;
    kPieceTypeToIndex[(int)'E'] = kPieceTypeToIndex[(int)'e'] = 1;
    kPieceTypeToIndex[(int)'H'] = kPieceTypeToIndex[(int)'h'] = 2;
    kPieceTypeToIndex[(int)'C'] = kPieceTypeToIndex[(int)'c'] = 2;
}

static void InitPieceToIndex(void) {
    for (int i = 0; i < kNumChars; ++i) {
        kPieceToIndex[i] = -1;
    }

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
    kPieceToIndex[(int)'-'] = 10;
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

static void DobutsuShogiInitGlobalVariables(void) {
    if (constants_initialized) return;

    InitPieceTypeToIndex();
    InitPieceToIndex();
    InitMoveMatrix();
    InitPromoteMatrix();
    constants_initialized = true;
}

// ========================== kDobutsuShogiSolverApi ==========================

static int64_t DobutsuShogiGetNumPositions(void) {
    return (int64_t)GenericHashNumPositions();
}

static Position DobutsuShogiGetInitialPosition(void) {
    /* g  l  e
       -  c  -
       -  C  -
       E  L  G */

    // 3 additional characters at the end are placeholders for unordered pieces.
    char initial_board[] = "gle-c--C-ELG---";
    initial_board[kBoardSize + 0] = 0;
    initial_board[kBoardSize + 1] = 0;
    initial_board[kBoardSize + 2] = 0;

    return GenericHashHash(initial_board, 1);
}

/**
 * @brief Returns true if \p src piece can capture \p dest piece, or false
 * otherwise.
 */
static bool CanCapture(char src, char dest) {
    return (dest == '-') || (isupper(src) && islower(dest)) ||
           (islower(src) && isupper(dest));
}

/**
 * @brief Returns the move of moving the piece from the \p src index to the \p
 * dest index. Note that \p dest is between 0 and 11, but \p src is between 0
 * and 14, where indices 12, 13, and 14 correspond to the captured pile of
 * (G)iraffes, (E)lephants, and (C)hicks respectively.
 */
static Move ConstructMove(int src, int dest) { return src * kBoardSize + dest; }

static void ExpandMove(Move move, int *src, int *dest) {
    *dest = move % kBoardSize;
    *src = move / kBoardSize;
}

static void ConvertCapturedToSky(char board[static kBoardStrSize]) {
    int count[4] = {2, 2, 2, 0};  // G, E, C, garbage
    for (int i = 0; i < kBoardSize; ++i) {
        --count[kPieceTypeToIndex[(int)board[i]]];
    }
    for (int i = 0; i < 3; ++i) {
        board[kBoardSize + i] = count[i] - board[kBoardSize + i];
    }
}

static MoveArray DobutsuShogiGenerateMoves(Position position) {
    MoveArray ret;
    MoveArrayInit(&ret);

    // Unhash
    char board[kBoardStrSize];
    bool success = GenericHashUnhash(position, board);
    assert(success);
    (void)success;
    bool p2_turn = GenericHashGetTurn(position) == 2;

    // Generate moves for board pieces
    for (int i = 0; i < kBoardSize; ++i) {
        if (isalpha(board[i]) && (p2_turn ^ (bool)isupper(board[i]))) {
            int piece_index = kPieceToIndex[(int)board[i]];
            for (int j = 0; j < kMoveMatrixNumMoves[piece_index][i]; ++j) {
                int dest = kMoveMatrix[piece_index][i][j];
                if (CanCapture(board[i], board[dest])) {
                    Move move = ConstructMove(i, dest);
                    MoveArrayAppend(&ret, move);
                }
            }
        }
    }

    // Generate moves for captured pieces.
    if (p2_turn) ConvertCapturedToSky(board);
    for (int i = kBoardSize; i < kBoardStrSize; ++i) {
        if (board[i]) {  // If there are previously captured pieces
            for (int j = 0; j < kBoardSize; ++j) {
                if (board[j] == '-') {  // If destination is empty
                    Move move = ConstructMove(i, j);
                    MoveArrayAppend(&ret, move);
                }
            }
        }
    }

    return ret;
}

static bool ImmediateCapture(Position position, int target) {
    bool ret = false;
    MoveArray moves = DobutsuShogiGenerateMoves(position);
    for (int64_t i = 0; i < moves.size; ++i) {
        int src = 0, dest = 0;
        ExpandMove(moves.array[i], &src, &dest);
        if (dest == target) {
            ret = true;
            break;
        }
    }
    MoveArrayDestroy(&moves);

    return ret;
}

static void CheckLions(const char board[static kBoardStrSize], bool *L_missing,
                       bool *l_missing) {
    int L = 0, l = 0;
    for (int i = 0; i < kBoardSize; ++i) {
        L += (board[i] == 'L');
        l += (board[i] == 'l');
    }

    *L_missing = (L == 0);
    *l_missing = (l == 0);
}

static int ForestTouchDown(const char board[static kBoardStrSize]) {
    for (int i = 0; i < 3; ++i) {
        if (board[i] == 'L') {
            return i;
        }
    }

    return -1;
}

static int SkyTouchDown(const char board[static kBoardStrSize]) {
    for (int i = 9; i < 12; ++i) {
        if (board[i] == 'l') {
            return i;
        }
    }

    return -1;
}

static Value DobutsuShogiPrimitive(Position position) {
    // Unhash
    char board[kBoardStrSize];
    bool success = GenericHashUnhash(position, board);
    assert(success);
    (void)success;

    // Check if one of the lions are missing.
    bool L_missing, l_missing;
    CheckLions(board, &L_missing, &l_missing);
    // The only possible case is that a lion was captured in the previous turn,
    // in which case it's a loss for the current player regardless of whose turn
    // it is.
    if (L_missing || l_missing) return kLose;

    // Check if one of the lions have reached the opponent's base.
    int i = ForestTouchDown(board);
    if (i >= 0) {
        return ImmediateCapture(position, i) ? kWin : kLose;
    }
    i = SkyTouchDown(board);
    if (i >= 0) {
        return ImmediateCapture(position, i) ? kWin : kLose;
    }

    return kUndecided;
}

static Position DoMoveInternal(const char board[static kBoardStrSize], int turn,
                               int src, int dest) {
    char board_copy[kBoardStrSize];
    memcpy(board_copy, board, kBoardStrSize);  // Make a copy of the board.

    // If capturing a sky player's (player 2's) non-lion piece, add it to the
    // pile of forest player's (player 1's) captured pieces.
    int dest_piece_type_idx = kPieceTypeToIndex[(int)board[dest]];
    if (dest_piece_type_idx <= 2 && islower(board[dest])) {
        ++board_copy[dest_piece_type_idx + kBoardSize];
    }

    if (src < kBoardSize) {  // Moving a piece on the board.
        int src_piece_idx = kPieceToIndex[(int)board_copy[src]];
        int chick_promote = ('H' - 'C') * kPromoteMatrix[src_piece_idx][dest];
        board_copy[dest] = board_copy[src] + chick_promote;
        board_copy[src] = '-';
    } else {  // Moving a piece from the captured pile.
        // kIndexToPieceType maps to capital letters. If it is the sky player's
        // (player 2's) turn, convert to lower case by adding 32 ('a' - 'A') to
        // the character.
        board_copy[dest] =
            kIndexToPieceType[src - kBoardSize] + ('a' - 'A') * (turn == 2);

        // If it is currently the forest player's (player 1's) turn and we are
        // removing a piece from the captured pile, decrement the corresponding
        // counter.
        board_copy[src] -= (turn == 1);
    }

    // Swap turn and hash.
    return GenericHashHash(board_copy, 3 - turn);
}

static Position DobutsuShogiDoMove(Position position, Move move) {
    // Unhash
    char board[kBoardStrSize];
    bool success = GenericHashUnhash(position, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurn(position);
    int src, dest;
    ExpandMove(move, &src, &dest);

    return DoMoveInternal(board, turn, src, dest);
}

static bool DobutsuShogiIsLegalPosition(Position position) {
    // Unhash
    char board[kBoardStrSize];
    bool success = GenericHashUnhash(position, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurn(position);

    // The game ends immediately after one of the lions are captured. Therefore,
    // it cannot be A's turn when B's lion is captured.
    bool L_missing, l_missing;
    CheckLions(board, &L_missing, &l_missing);
    if (L_missing && turn == 2) return false;
    if (l_missing && turn == 1) return false;

    // The game ends immediately after one of the lions reaches the opponent's
    // base. Therefore, it cannot be A's turn if A's lion has reached the base.
    bool L_touchdown = (ForestTouchDown(board) >= 0);
    bool l_touchdown = (SkyTouchDown(board) >= 0);
    if (L_touchdown && turn == 1) return false;
    if (l_touchdown && turn == 2) return false;
    // Both players cannot reach each other's base at the same time, but this is
    // already guaranteed by the 2 conditions above.
    assert(!(L_touchdown && l_touchdown));

    return true;
}

static Position DobutsuShogiGetCanonicalPosition(Position position) {
    // Unhash
    char board[kBoardStrSize], sym_board[kBoardStrSize];
    bool success = GenericHashUnhash(position, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurn(position);

    // Convert board to the symmetric positions (only 1 for now).
    Position ret = position;
    for (int sym = 1; sym < kNumSymmetries; ++sym) {
        for (int slot = 0; slot < kBoardSize; ++slot) {
            sym_board[slot] = board[kSymmetryMatrix[sym][slot]];
        }
        for (int slot = kBoardSize; slot < kBoardStrSize; ++slot) {
            sym_board[slot] = board[slot];
        }

        Position sym_pos = GenericHashHash(sym_board, turn);
        if (sym_pos < ret) ret = sym_pos;
    }

    return ret;
}

static void AddIfNotDuplicate(PositionArray *array, PositionHashSet *dedup_set,
                              Position pos) {
    pos = DobutsuShogiGetCanonicalPosition(pos);
    if (!PositionHashSetContains(dedup_set, pos)) {
        PositionHashSetAdd(dedup_set, pos);
        PositionArrayAppend(array, pos);
    }
}

static PositionArray DobutsuShogiGetCanonicalChildPositions(Position position) {
    // Unhash
    char board[kBoardStrSize];
    bool success = GenericHashUnhash(position, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurn(position);
    bool p2_turn = (turn == 2);
    PositionArray ret;
    PositionArrayInit(&ret);
    PositionHashSet dedup_set;
    PositionHashSetInit(&dedup_set, 0.5);

    // Generate children by moving board pieces
    for (int i = 0; i < kBoardSize; ++i) {
        if (isalpha(board[i]) && (p2_turn ^ (bool)isupper(board[i]))) {
            int piece_index = kPieceToIndex[(int)board[i]];
            for (int j = 0; j < kMoveMatrixNumMoves[piece_index][i]; ++j) {
                int dest = kMoveMatrix[piece_index][i][j];
                if (CanCapture(board[i], board[dest])) {
                    Position child = DoMoveInternal(board, turn, i, dest);
                    AddIfNotDuplicate(&ret, &dedup_set, child);
                }
            }
        }
    }

    // Generate moves for captured pieces.
    char board_copy[kBoardStrSize];
    memcpy(board_copy, board, kBoardStrSize);
    if (p2_turn) ConvertCapturedToSky(board_copy);
    for (int i = kBoardSize; i < kBoardStrSize; ++i) {
        if (board_copy[i]) {  // If there are previously captured pieces
            for (int j = 0; j < kBoardSize; ++j) {
                if (board_copy[j] == '-') {  // If destination is empty
                    Position child = DoMoveInternal(board, turn, i, j);
                    AddIfNotDuplicate(&ret, &dedup_set, child);
                }
            }
        }
    }
    PositionHashSetDestroy(&dedup_set);

    return ret;
}

static const RegularSolverApi kDobutsuShogiSolverApi = {
    .GetNumPositions = DobutsuShogiGetNumPositions,
    .GetInitialPosition = DobutsuShogiGetInitialPosition,

    .GenerateMoves = DobutsuShogiGenerateMoves,
    .Primitive = DobutsuShogiPrimitive,
    .DoMove = DobutsuShogiDoMove,
    .IsLegalPosition = DobutsuShogiIsLegalPosition,
    .GetCanonicalPosition = DobutsuShogiGetCanonicalPosition,
    .GetCanonicalChildPositions = DobutsuShogiGetCanonicalChildPositions,
};

// ========================= kDobutsuShogiGameplayApi =========================

static const char kPositionStringFormat[] =
    "\n"
    "P2 Captured:                      %c %c %c %c %c %c\n"
    "------------------------ Sky ----------------------\n"
    "\n"
    "         (  1  2  3 )           : %c %c %c\n"
    "         (  4  5  6 )           : %c %c %c\n"
    "LEGEND:  (  7  8  9 )  TOTAL:   : %c %c %c\n"
    "         ( 10 11 12 )           : %c %c %c\n"
    "\n"
    "----------------------- Forest --------------------\n"
    "P1 Captured:                      %c %c %c %c %c %c\n"
    "\n";

static int DobutsuShogiPositionToString(Position position, char *buffer) {
    // Unhash
    char board[kBoardStrSize];
    if (!GenericHashUnhash(position, board)) return kGenericHashError;

    char p1_captured[7] = "      ", p2_captured[7] = "      ";
    // Collect p1 captured pieces.
    for (int i = 0; i < 3; ++i) {
        char to_print = kIndexToPieceType[i];
        switch (board[kBoardSize + i]) {
            case 1:
                p1_captured[i * 2] = to_print;
                break;
            case 2:
                p1_captured[i * 2] = p1_captured[i * 2 + 1] = to_print;
                break;
            default:
                break;
        }
    }
    // Collect p2 captured pieces.
    ConvertCapturedToSky(board);
    for (int i = 0; i < 3; ++i) {
        char to_print = tolower(kIndexToPieceType[i]);
        switch (board[kBoardSize + i]) {
            case 1:
                p2_captured[i * 2] = to_print;
                break;
            case 2:
                p2_captured[i * 2] = p2_captured[i * 2 + 1] = to_print;
                break;
            default:
                break;
        }
    }
    sprintf(buffer, kPositionStringFormat, p2_captured[0], p2_captured[1],
            p2_captured[2], p2_captured[3], p2_captured[4], p2_captured[5],
            board[0], board[1], board[2], board[3], board[4], board[5],
            board[6], board[7], board[8], board[9], board[10], board[11],
            p1_captured[0], p1_captured[1], p1_captured[2], p1_captured[3],
            p1_captured[4], p1_captured[5]);

    return kNoError;
}

static int DobutsuShogiMoveToString(Move move, char *buffer) {
    int src = 0, dest = 0;
    ExpandMove(move, &src, &dest);
    char src_str[kInt32Base10StringLengthMax + 1];
    if (src < kBoardSize) {
        sprintf(src_str, "%d", src + 1);  // Print in 1-index format.
    } else {
        sprintf(src_str, "%c", tolower(kIndexToPieceType[src - kBoardSize]));
    }

    sprintf(buffer, "%s %d", src_str, dest + 1);

    return kNoError;
}

static bool DobutsuShogiIsValidMoveString(ReadOnlyString move_string) {
    size_t len = strlen(move_string);
    if (len < 3 || len > 5) return false;

    // The first char must be one of 'g', 'e', 'c', or a non-zero digit
    if (move_string[0] != 'g' && move_string[0] != 'e' &&
        move_string[0] != 'c' && !isdigit(move_string[0])) {
        return false;
    }
    if (move_string[0] == '0') return false;

    // If the first char is one of 'g', 'e', 'c', the second char must be a
    // white space.
    if (isalpha(move_string[0]) && move_string[1] != ' ') return false;

    char buffer[6];
    strcpy(buffer, move_string);
    if (isalpha(move_string[0])) {
        // If begins with one of 'g', 'e', 'c', validate only the dest index.
        strtok(buffer, " ");  // Get rid of the first chunk
    } else {
        // Otherwise, we also need to validate src.
        int src = atoi(strtok(buffer, " "));
        if (src < 1 || src > kBoardSize) return false;
    }

    int dest = atoi(strtok(NULL, " "));
    if (dest < 1 || dest > kBoardSize) return false;

    return true;
}

static Move DobutsuShogiStringToMove(ReadOnlyString move_string) {
    int src, dest;
    if (isalpha(move_string[0])) {
        src = kPieceTypeToIndex[(int)move_string[0]] + kBoardSize;
        dest = atoi(move_string + 2) - 1;  // 1-indexed.
    } else {
        char buffer[6];
        strcpy(buffer, move_string);
        src = atoi(strtok(buffer, " ")) - 1;  // 1-indexed.
        dest = atoi(strtok(NULL, " ")) - 1;   // 1-indexed.
    }

    return ConstructMove(src, dest);
}

static const GameplayApiCommon kDobutsuShogiGameplayApiCommon = {
    .GetInitialPosition = DobutsuShogiGetInitialPosition,
    .position_string_length_max = sizeof(kPositionStringFormat),

    .move_string_length_max = 6,
    .MoveToString = DobutsuShogiMoveToString,

    .IsValidMoveString = DobutsuShogiIsValidMoveString,
    .StringToMove = DobutsuShogiStringToMove,
};

static const GameplayApiRegular kDobutsuShogiGameplayApiRegular = {
    .PositionToString = DobutsuShogiPositionToString,

    .GenerateMoves = DobutsuShogiGenerateMoves,
    .DoMove = DobutsuShogiDoMove,
    .Primitive = DobutsuShogiPrimitive,
};

static const GameplayApi kDobutsuShogiGameplayApi = {
    .common = &kDobutsuShogiGameplayApiCommon,
    .regular = &kDobutsuShogiGameplayApiRegular,
};

// ========================= kDobutsuShogiUwapiRegular =========================

// Formal position format:
// """
// [turn]_[board (12x)]_[G count][E count][C count]_[g count][e count][c count]
// """

// This function is implemented in the code section for DobutsuShogiInit.
static bool DobutsuShogiIsValidConfig(const int *config);

static bool DobutsuShogiIsLegalFormalPosition(ReadOnlyString formal_position) {
    // String length must match regular format.
    if (strlen(formal_position) != kDobutsuShogiFormalPositionStrlen) {
        return false;
    }

    // The first char must be either 1 or 2.
    if (formal_position[0] != '1' && formal_position[0] != '2') return false;

    int config[14] = {0};
    char board[kBoardStrSize];
    for (int i = 0; i < kBoardSize; ++i) {
        board[i] = formal_position[i + 2];
        // Each board piece char must be valid.
        if (kPieceToIndex[(int)board[i]] < 0) return false;
        ++config[kPieceToIndex[(int)board[i]]];
    }
    for (int i = kBoardSize; i < kBoardStrSize; ++i) {
        board[i] = config[i - 1] = formal_position[i + 3] - '0';
    }

    // The piece configuration must be valid.
    if (!DobutsuShogiIsValidConfig(config)) return false;

    // Check the number of pieces captured by the sky player.
    ConvertCapturedToSky(board);
    for (int i = 0; i < 3; ++i) {
        if (board[i + kBoardSize] != formal_position[19 + i] - '0') {
            return false;
        }
    }

    return true;
}

static Position DobutsuShogiFormalPositionToPosition(
    ReadOnlyString formal_position) {
    //
    char board[kBoardStrSize];
    memcpy(board, formal_position + 2, kBoardSize);
    for (int i = kBoardSize; i < kBoardStrSize; ++i) {
        board[i] = formal_position[i + 3] - '0';
    }

    return GenericHashHash(board, formal_position[0] - '0');
}

static CString DobutsuShogiPositionToFormalPosition(Position position) {
    // Unhash
    char board[kBoardStrSize];
    bool success = GenericHashUnhash(position, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurn(position);
    char formal_position[] = "1_------------_000_000";  // Placeholder.
    formal_position[0] = '0' + turn;
    memcpy(formal_position + 2, board, kBoardSize);
    for (int i = kBoardSize; i < kBoardStrSize; ++i) {
        formal_position[i + 3] = board[i] + '0';
    }
    ConvertCapturedToSky(board);
    for (int i = kBoardSize; i < kBoardStrSize; ++i) {
        formal_position[i + 7] = board[i] + '0';
    }

    CString ret;
    CStringInitCopy(&ret, formal_position);

    return ret;
}

// AutoGui position format:
// """
// [turn]_[board (12x)][-/G/B][-/E/A][-/C/D][-/g/b][-/e/a][-/c/d]
// """

static const char kAutoGuiCapturedCharMap[6][3] = {
    {'-', 'G', 'B'}, {'-', 'E', 'A'}, {'-', 'C', 'D'},
    {'-', 'g', 'b'}, {'-', 'e', 'a'}, {'-', 'c', 'd'},
};

static CString DobutsuShogiPositionToAutoGuiPosition(Position position) {
    // Unhash
    char board[kBoardStrSize];
    bool success = GenericHashUnhash(position, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurn(position);
    char autogui_position[] = "1_------------GECgec";  // Placeholder.
    autogui_position[0] = '0' + turn;
    memcpy(autogui_position + 2, board, kBoardSize);
    for (int i = 0; i < 3; ++i) {
        autogui_position[2 + kBoardSize + i] =
            kAutoGuiCapturedCharMap[i][(int)board[kBoardSize + i]];
    }
    ConvertCapturedToSky(board);
    for (int i = 0; i < 3; ++i) {
        autogui_position[2 + kBoardSize + 3 + i] =
            kAutoGuiCapturedCharMap[i + 3][(int)board[kBoardSize + i]];
    }

    CString ret;
    CStringInitCopy(&ret, autogui_position);

    return ret;
}

static const ConstantReadOnlyString kFormalMoveMap[12] = {
    "A1", "B1", "C1", "A2", "B2", "C2", "A3", "B3", "C3", "A4", "B4", "C4",
};

static CString DobutsuShogiMoveToFormalMove(Position position, Move move) {
    int src = 0, dest = 0;
    ExpandMove(move, &src, &dest);
    char src_str[kInt32Base10StringLengthMax + 2];
    if (src < kBoardSize) {
        sprintf(src_str, "%s",
                kFormalMoveMap[src]);  // Print in 1-index format.
    } else {
        char piece_type = kIndexToPieceType[src - kBoardSize];
        if (GenericHashGetTurn(position) == 2) piece_type = tolower(piece_type);
        sprintf(src_str, "%c", piece_type);
    }

    char buffer[kInt32Base10StringLengthMax + 5];
    sprintf(buffer, "%s %s", src_str, kFormalMoveMap[dest]);
    CString ret;
    CStringInitCopy(&ret, buffer);

    return ret;
}

static CString DobutsuShogiMoveToAutoGuiMove(Position position, Move move) {
    (void)position;
    int src = 0, dest = 0;
    ExpandMove(move, &src, &dest);
    CString ret;
    char autogui_move[6 + 2 * kInt32Base10StringLengthMax];
    if (src < kBoardSize) {  // M-Type move from src to dest.
        sprintf(autogui_move, "M_%d_%d_x", src, dest);
    } else {  // A-Type move adding piece to dest.
        // This is the center index in the AutoGUI coordinate system.
        int center = (src - kBoardSize) * kBoardSize + dest + kBoardSize + 6;
        sprintf(autogui_move, "A_%d_%d_y", src - kBoardSize, center);
    }
    CStringInitCopy(&ret, autogui_move);

    return ret;
}

static const UwapiRegular kDobutsuShogiUwapiRegular = {
    .GenerateMoves = DobutsuShogiGenerateMoves,
    .DoMove = DobutsuShogiDoMove,
    .Primitive = DobutsuShogiPrimitive,

    .IsLegalFormalPosition = DobutsuShogiIsLegalFormalPosition,
    .FormalPositionToPosition = DobutsuShogiFormalPositionToPosition,
    .PositionToFormalPosition = DobutsuShogiPositionToFormalPosition,
    .PositionToAutoGuiPosition = DobutsuShogiPositionToAutoGuiPosition,
    .MoveToFormalMove = DobutsuShogiMoveToFormalMove,
    .MoveToAutoGuiMove = DobutsuShogiMoveToAutoGuiMove,
    .GetInitialPosition = DobutsuShogiGetInitialPosition,
    .GetRandomLegalPosition = NULL,  // Not available for this game.
};

static const Uwapi kQuixoUwapi = {.regular = &kDobutsuShogiUwapiRegular};

// ============================= DobutsuShogiInit =============================

static bool DobutsuShogiIsValidConfig(const int *config) {
    // No need to check if elements add up to 12 here because it's done by the
    // Generic Hash module automatically.
    if (config[0] + config[1] < 1) {
        // At least one lion.
        return false;
    } else if (config[2] + config[3] + config[11] > 2) {
        // At most two giraffes.
        return false;
    } else if (config[4] + config[5] + config[12] > 2) {
        // At most two elephants.
        return false;
    } else if (config[6] + config[7] + config[8] + config[9] + config[13] > 2) {
        // At most two chicks/hens.
        return false;
    }

    return true;
}

static int DobutsuShogiInit(void *aux) {
    (void)aux;  // Unused.
    static const int kPiecesInit[] = {
        // Board pieces
        'L', 0, 1, 'l', 0, 1, 'G', 0, 2, 'g', 0, 2, 'E', 0, 2, 'e', 0, 2, 'H',
        0, 2, 'h', 0, 2, 'C', 0, 2, 'c', 0, 2, '-', 4, 11, -2,

        // Unordered pieces: captured pieces by the forest player.
        // For consistency, they are (G)iraffes, (E)lephants, and (C)hicks
        // respectively in this order throughout this entire file.
        0, 2, 0, 2, 0, 2, -1};

    GenericHashReinitialize();
    bool success = GenericHashAddContext(0, kBoardSize, kPiecesInit,
                                         DobutsuShogiIsValidConfig, 0);
    if (!success) {
        fprintf(stderr,
                "DobutsuShogiInit: failed to initialize generic hash\n");
        return kGenericHashError;
    }

    DobutsuShogiInitGlobalVariables();

    return kNoError;
}

// =========================== DobutsuShogiFinalize ===========================

static int DobutsuShogiFinalize(void) { return kNoError; }

// =============================== kDobutsuShogi ===============================

/** @brief Dōbutsu shōgi. */
const Game kDobutsuShogi = {
    .name = "dshogi",
    .formal_name = "Dōbutsu shōgi",
    .solver = &kRegularSolver,
    .solver_api = &kDobutsuShogiSolverApi,
    .gameplay_api = &kDobutsuShogiGameplayApi,
    .uwapi = &kQuixoUwapi,

    .Init = DobutsuShogiInit,
    .Finalize = DobutsuShogiFinalize,

    .GetCurrentVariant = NULL,  // No other variants
    .SetVariantOption = NULL,   // No other variants
};

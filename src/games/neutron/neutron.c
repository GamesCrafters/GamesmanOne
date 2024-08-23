/**
 * @file neutron.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of Neutron.
 * @details https://www.di.fc.ul.pt/~jpn/gv/neutron.htm
 * @version 1.0.0
 * @date 2024-08-22
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

#include "games/neutron/neutron.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdint.h>  // int64_t
#include <stdlib.h>  // atoi, strtok
#include <string.h>  // strlen

#include "core/generic_hash/generic_hash.h"
#include "core/misc.h"
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/types/gamesman_types.h"

// ============================= Global Constants =============================

enum {
    kBoardRows = 5,
    kBoardCols = 5,
    kBoardSize = kBoardRows * kBoardCols,
};

/*
 * O O O O O
 * - - - - -
 * - - N - -
 * - - - - -
 * X X X X X
 */
static ConstantReadOnlyString kInitialBoard = "OOOOO-------N-------XXXXX";
static const int kSymmetry[] = {
    4,  3,  2,  1,  0,  9,  8,  7,  6,  5,  14, 13, 12,
    11, 10, 19, 18, 17, 16, 15, 24, 23, 22, 21, 20,
};

static const int kXHomeRow[] = {20, 21, 22, 23, 24};
static const int kOHomeRow[] = {0, 1, 2, 3, 4};
static const int kDirections[8][2] = {
    {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1},
};
static ConstantReadOnlyString kDirectionStr[8] = {
    "UL", "U", "UR", "L", "R", "DL", "D", "DR",
};

// Hash for the initial position, which is manually assigned to max generic hash
// value + 1. Although this value is set at game-initialization phase, its value
// should be considered constant.
static Position kInitialPosition;

static const int *kXWinningRow = kXHomeRow;
static const int *kOWinningRow = kOHomeRow;

// ============================= kNeutronSolverApi =============================

static int64_t NeutronGetNumPositions(void) {
    // +1 for the initial position, which is special because the first move does
    // not involve the neutron.
    return (int64_t)GenericHashNumPositions() + 1;
}

static Position NeutronGetInitialPosition(void) { return kInitialPosition; }

// Pass n_src == -1 for initial position.
static Move ConstructMove(int n_src, int n_dir, int p_src, int p_dir) {
    // 26 possible neutron sources (1 additional for initial position's neutron,
    // which cannot be moved), 25 piece sources, and 8 possible directions for
    // each part.
    return (((n_src + 1) * 8 + n_dir) * kBoardSize + p_src) * 8 + p_dir;
}

static void ExpandMove(Move move, int *n_src, int *n_dir, int *p_src,
                       int *p_dir) {
    *p_dir = move % 8;
    move /= 8;

    *p_src = move % kBoardSize;
    move /= kBoardSize;

    *n_dir = move % 8;
    move /= 8;

    *n_src = move - 1;
}

// Returns the index of the neutron's location.
static int FindNeutron(const char board[static kBoardSize]) {
    for (int i = 0; i < kBoardSize; ++i) {
        if (board[i] == 'N') return i;
    }

    NotReached("NeutronGenerateMoves: failed to find the neutron on board");
    return -1;
}

static bool OnBoard(int row, int col) {
    if (row < 0) return false;
    if (col < 0) return false;
    if (row >= kBoardRows) return false;
    if (col >= kBoardCols) return false;

    return true;
}

static int ToIndex(int row, int col) { return row * kBoardCols + col; }

static const char kTurnToPiece[] = {'-', 'X', 'O', 'N'};

static bool CanMoveInDirection(const char board[static kBoardSize], int src,
                               int dir) {
    int row = src / kBoardCols;
    int col = src % kBoardCols;
    int row_off = kDirections[dir][0];
    int col_off = kDirections[dir][1];

    // If the piece can be moved for at least one sqaure, then the move is
    // valid. Here, we call the square being checked that is one square away
    // from the source "the destination" for simplicity, even though the
    // piece should move all the way in that direction until it hits a wall
    // or another piece.
    int dest_row = row + row_off;
    int dest_col = col + col_off;

    // Cannot move outside the board.
    if (!OnBoard(dest_row, dest_col)) return false;

    // Cannot make the move if the direction is blocked by another piece.
    if (board[ToIndex(dest_row, dest_col)] != '-') return false;

    return true;
}

// Returns destination index.
static int MovePiece(char board[static kBoardSize], int src, int dir) {
    int row = src / kBoardCols;
    int col = src % kBoardCols;
    int row_off = kDirections[dir][0];
    int col_off = kDirections[dir][1];
    int dest_row = row + row_off;
    int dest_col = col + col_off;
    int dest = ToIndex(dest_row, dest_col);
    while (OnBoard(dest_row, dest_col) && board[dest] == '-') {
        dest_row += row_off;
        dest_col += col_off;
        dest = ToIndex(dest_row, dest_col);
    }

    dest_row -= row_off;
    dest_col -= col_off;
    dest = ToIndex(dest_row, dest_col);
    assert(dest != src);
    board[dest] = board[src];
    board[src] = '-';

    return dest;
}

static void GeneratePieceMoves(const char board[static kBoardSize], int turn,
                               int n_src, int n_dir, MoveArray *moves) {
    char piece_to_move = kTurnToPiece[turn];
    for (int i = 0; i < kBoardSize; ++i) {
        if (board[i] != piece_to_move) continue;
        for (int dir = 0; dir < 8; ++dir) {
            if (CanMoveInDirection(board, i, dir)) {
                Move move = ConstructMove(n_src, n_dir, i, dir);
                MoveArrayAppend(moves, move);
            }
        }
    }
}

static bool Unhash(Position hash, char board[static kBoardSize]) {
    if (hash == kInitialPosition) {
        memcpy(board, kInitialBoard, kBoardSize);
        return true;
    }

    return GenericHashUnhash(hash, board);
}

static int GetTurn(Position hash) {
    if (hash == kInitialPosition) return 1;

    return GenericHashGetTurn(hash);
}

static MoveArray NeutronGenerateMoves(Position position) {
    MoveArray ret;
    MoveArrayInit(&ret);

    // Unhash
    char board[kBoardSize];
    bool success = Unhash(position, board);
    assert(success);
    (void)success;
    int turn = GetTurn(position);

    // If the given position is the initial position, generate piece moves
    // directly.
    if (position == kInitialPosition) {
        GeneratePieceMoves(board, turn, -1, 0, &ret);
        return ret;
    }

    int n_src = FindNeutron(board);

    // For each possible neutron move
    for (int n_dir = 0; n_dir < 8; ++n_dir) {
        // If cannot move the neutron in this direction, skip it.
        if (!CanMoveInDirection(board, n_src, n_dir)) continue;

        // Make the current neutron move and generate piece moves.
        int dest = MovePiece(board, n_src, n_dir);
        GeneratePieceMoves(board, turn, n_src, n_dir, &ret);

        // Revert the neutron move.
        board[n_src] = 'N';
        board[dest] = '-';
    }

    return ret;
}

static Value NeutronPrimitive(Position position) {
    // Unhash
    char board[kBoardSize];
    bool success = Unhash(position, board);
    assert(success);
    (void)success;
    int turn = GetTurn(position);
    bool x_turn = (turn == 1);
    bool o_turn = (turn == 2);

    // Check if the neutron has reached one of the home rows.
    for (int i = 0; i < 5; ++i) {
        if (board[kXWinningRow[i]] == 'N') {
            return x_turn ? kWin : kLose;
        }
        if (board[kOWinningRow[i]] == 'N') {
            return o_turn ? kWin : kLose;
        }
    }

    // Check if the current player is in a stalemate.
    MoveArray moves = NeutronGenerateMoves(position);
    if (moves.size == 0) return kLose;

    return kUndecided;
}

static Position NeutronDoMove(Position position, Move move) {
    // Unhash
    char board[kBoardSize];
    bool success = Unhash(position, board);
    assert(success);
    (void)success;
    int turn = GetTurn(position);

    int n_src, n_dir, p_src, p_dir;
    ExpandMove(move, &n_src, &n_dir, &p_src, &p_dir);
    if (n_src >= 0) {  // Not initial position, perform neutron move.
        assert(position != kInitialPosition);
        MovePiece(board, n_src, n_dir);
    }

    // Always perform piece move.
    MovePiece(board, p_src, p_dir);

    return GenericHashHash(board, 3 - turn);
}

static bool NeutronIsLegalPosition(Position position) {
    (void)position;  // No simple way to check for illegal positions.
    return true;
}

static Position NeutronGetCanonicalPosition(Position position) {
    // The initial position happens to be a canonical position by the only
    // available reflection symmetry.
    if (position == kInitialPosition) return position;

    // Unhash
    char board[kBoardSize], sym_board[kBoardSize];
    // This call to GenericHashUnhash is safe because we already checked for the
    // initial position.
    bool success = GenericHashUnhash(position, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurn(position);

    // Apply the only symmetry.
    for (int i = 0; i < kBoardSize; ++i) {
        sym_board[i] = board[kSymmetry[i]];
    }
    Position sym = GenericHashHash(sym_board, turn);

    return position < sym ? position : sym;
}

// static PositionArray NeutronGetCanonicalChildPositions(Position position) {}

static const RegularSolverApi kNeutronSolverApi = {
    .GetNumPositions = NeutronGetNumPositions,
    .GetInitialPosition = NeutronGetInitialPosition,

    .GenerateMoves = NeutronGenerateMoves,
    .Primitive = NeutronPrimitive,
    .DoMove = NeutronDoMove,
    .IsLegalPosition = NeutronIsLegalPosition,
    .GetCanonicalPosition = NeutronGetCanonicalPosition,
    .GetCanonicalChildPositions = NULL,  // TODO
};

// ============================ kNeutronGameplayApi ============================

static const char kPositionStringFormat[] =
    "         (  1  2  3  4  5 )           : %c %c %c %c %c\n"
    "         (  6  7  8  9 10 )           : %c %c %c %c %c\n"
    "LEGEND:  ( 11 12 13 14 15 )  TOTAL:   : %c %c %c %c %c\n"
    "         ( 16 17 18 19 20 )           : %c %c %c %c %c\n"
    "         ( 21 22 23 24 25 )           : %c %c %c %c %c\n";

static int NeutronPositionToString(Position position, char *buffer) {
    // Unhash
    char board[kBoardSize];
    bool success = Unhash(position, board);
    if (!success) return kGenericHashError;

    sprintf(buffer, kPositionStringFormat, board[0], board[1], board[2],
            board[3], board[4], board[5], board[6], board[7], board[8],
            board[9], board[10], board[11], board[12], board[13], board[14],
            board[15], board[16], board[17], board[18], board[19], board[20],
            board[21], board[22], board[23], board[24]);

    return kNoError;
}

static int NeutronMoveToString(Move move, char *buffer) {
    int n_src, n_dir, p_src, p_dir;
    ExpandMove(move, &n_src, &n_dir, &p_src, &p_dir);
    if (n_src < 0) {  // No neutron move
        sprintf(buffer, "%d %s", p_src + 1, kDirectionStr[p_dir]);
    } else {
        sprintf(buffer, "%d %s %d %s", n_src + 1, kDirectionStr[n_dir],
                p_src + 1, kDirectionStr[p_dir]);
    }

    return kNoError;
}

static int DirectionStrToDir(const char *dir_str) {
    for (int i = 0; i < 8; ++i) {
        if (strcmp(dir_str, kDirectionStr[i]) == 0) return i;
    }

    return -1;
}

static bool IsValidMoveTokenPair(const char *token1, const char *token2) {
    if (token1 == NULL || token2 == NULL) return false;

    size_t len = strlen(token1);
    if (len < 1 || len > 2) return false;
    len = strlen(token2);
    if (len < 1 || len > 2) return false;

    int src = atoi(token1);
    if (src < 1 || src > kBoardSize) return false;

    int dir = DirectionStrToDir(token2);
    if (dir < 0) return false;

    return true;
}

static ConstantReadOnlyString kMoveStrDelim = " ";

static bool NeutronIsValidMoveString(ReadOnlyString move_string) {
    // Valid move formats:
    // 1. Initial position: "[p_src] [p_dir]"
    // 2. Any other position: "[n_src] [n_dir] [p_src] [p_dir]"
    if (move_string == NULL) return false;
    size_t len = strlen(move_string);
    if (len < 4 || len > 11) return false;

    char move_string_copy[12];
    strcpy(move_string_copy, move_string);
    char *tokens[4];
    tokens[0] = strtok(move_string_copy, kMoveStrDelim);
    for (int i = 1; i < 4; ++i) {
        tokens[i] = strtok(NULL, kMoveStrDelim);
    }

    // Validate the first two tokens
    if (!IsValidMoveTokenPair(tokens[0], tokens[1])) return false;

    // If no more tokens, the format is valid for the initial position.
    if (tokens[2] == NULL) return tokens[3] == NULL;

    // Otherwise, also check for the last two tokens.
    if (!IsValidMoveTokenPair(tokens[2], tokens[3])) return false;

    return true;
}

static Move NeutronStringToMove(ReadOnlyString move_string) {
    char move_string_copy[12];
    strcpy(move_string_copy, move_string);
    char *tokens[4];
    tokens[0] = strtok(move_string_copy, kMoveStrDelim);
    for (int i = 1; i < 4; ++i) {
        tokens[i] = strtok(NULL, kMoveStrDelim);
    }

    int n_src, n_dir, p_src, p_dir;
    if (tokens[2] == NULL) {  // Move at the initial position.
        assert(tokens[3] == NULL);
        n_src = -1;
        n_dir = 0;
        p_src = atoi(tokens[0]) - 1;
        p_dir = DirectionStrToDir(tokens[1]);
    } else {  // Move at any other position.
        assert(tokens[3] != NULL);
        n_src = atoi(tokens[0]) - 1;
        n_dir = DirectionStrToDir(tokens[1]);
        p_src = atoi(tokens[2]) - 1;
        p_dir = DirectionStrToDir(tokens[3]);
    }

    return ConstructMove(n_src, n_dir, p_src, p_dir);
}

static const GameplayApiCommon kNeutronGameplayApiCommon = {
    .GetInitialPosition = NeutronGetInitialPosition,
    .position_string_length_max = sizeof(kPositionStringFormat),

    .move_string_length_max = 11,
    .MoveToString = NeutronMoveToString,

    .IsValidMoveString = NeutronIsValidMoveString,
    .StringToMove = NeutronStringToMove,
};

static const GameplayApiRegular kNeutronGameplayApiRegular = {
    .PositionToString = NeutronPositionToString,

    .GenerateMoves = NeutronGenerateMoves,
    .DoMove = NeutronDoMove,
    .Primitive = NeutronPrimitive,
};

static const GameplayApi kNeutronGameplayApi = {
    .common = &kNeutronGameplayApiCommon,
    .regular = &kNeutronGameplayApiRegular,
};

static int NeutronInit(void *aux) {
    (void)aux;  // Unused.
    GenericHashReinitialize();

    // Using X for the first player's pieces, O for the second player's pieces,
    // and N for the neutron.
    int pieces_init[] = {'X', 5, 5, 'O', 5, 5, 'N', 1, 1, '-', 14, 14, -1};
    GenericHashAddContext(0, kBoardSize, pieces_init, NULL, 0);
    kInitialPosition = GenericHashNumPositions();

    return kNoError;
}

static int NeutronFinalize(void) { return kNoError; }

/** @brief Neutron */
const Game kNeutron = {
    .name = "neutron",
    .formal_name = "Neutron",
    .solver = &kRegularSolver,
    .solver_api = &kNeutronSolverApi,
    .gameplay_api = &kNeutronGameplayApi,
    .uwapi = NULL,  // TODO

    .Init = NeutronInit,
    .Finalize = NeutronFinalize,

    .GetCurrentVariant = NULL,  // No other variants
    .SetVariantOption = NULL,   // No other variants
};

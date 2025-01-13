/**
 * @file neutron.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of Neutron.
 * @details https://www.di.fc.ul.pt/~jpn/gv/neutron.htm
 * @version 1.1.0
 * @date 2024-11-14
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
#include <ctype.h>   // toupper
#include <stddef.h>  // NULL
#include <stdint.h>  // int64_t
#include <stdlib.h>  // atoi
#include <string.h>  // strlen, strtok_r

#include "core/hash/generic.h"
#include "core/misc.h"
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/types/gamesman_types.h"

// ============================= Type Definitions =============================

// clang-tidy gives many warnings about narrowing conversion from 'int' to
// 'int8_t' that are known to be correct. The following line turns them all
// off. Consider re-enabling them before implementing new features.
// NOLINTBEGIN(cppcoreguidelines-narrowing-conversions)

typedef union {
    struct {
        /** [-1, 25), 25 possible neutron move sources plus an additional value
         * -1 for no neutron move at the initial position. */
        int8_t n_src;

        /** [0, 8), 8 possible neutron move directions. */
        int8_t n_dir;

        /** [-1, 25), 25 possible piece move sources plus an additional value
         * -1 for no neutron move due to game over after moving the neutron. */
        int8_t p_src;

        /** [0, 8), 8 possible piece move directions. */
        int8_t p_dir;
    } unpacked;
    Move hashed;
} NeutronMove;

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
static const int8_t kSymmetry[] = {
    4,  3,  2,  1,  0,  9,  8,  7,  6,  5,  14, 13, 12,
    11, 10, 19, 18, 17, 16, 15, 24, 23, 22, 21, 20,
};

static const int8_t kXHomeRow[] = {20, 21, 22, 23, 24};
static const int8_t kOHomeRow[] = {0, 1, 2, 3, 4};
static const int8_t kDirections[8][2] = {
    {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1},
};
static ConstantReadOnlyString kDirectionStr[8] = {
    "UL", "U", "UR", "L", "R", "DL", "D", "DR",
};

// Hash for the initial position, which is manually assigned to max generic hash
// value + 1. Although this value is set at game-initialization phase, its value
// should be considered constant.
static Position kInitialPosition;

static PositionHashSet kChildrenOfInitialPosition;

static const int8_t *kXWinningRow = kXHomeRow;
static const int8_t *kOWinningRow = kOHomeRow;

static const NeutronMove kNeutronMoveInit = {
    .unpacked =
        {
            .n_src = -1,
            .n_dir = 0,
            .p_src = -1,
            .p_dir = 0,
        },
};

// ============================= kNeutronSolverApi =============================

static int64_t NeutronGetNumPositions(void) {
    // +1 for the initial position, which is special because the first move does
    // not involve the neutron.
    return (int64_t)GenericHashNumPositions() + 1;
}

static Position NeutronGetInitialPosition(void) { return kInitialPosition; }

// Returns the index of the neutron's location.
static int8_t FindNeutron(const char board[static kBoardSize]) {
    for (int8_t i = 0; i < kBoardSize; ++i) {
        if (board[i] == 'N') return i;
    }

    NotReached("FindNeutron: failed to find the neutron on board");
    return -1;
}

static bool OnBoard(int8_t row, int8_t col) {
    if (row < 0) return false;
    if (col < 0) return false;
    if (row >= kBoardRows) return false;
    if (col >= kBoardCols) return false;

    return true;
}

static int8_t ToIndex(int8_t row, int8_t col) { return row * kBoardCols + col; }

static const char kTurnToPiece[] = {'-', 'X', 'O', 'N'};

static bool CanMoveInDirection(const char board[static kBoardSize], int8_t src,
                               int8_t dir) {
    int8_t row = src / kBoardCols;
    int8_t col = src % kBoardCols;
    int8_t row_off = kDirections[dir][0];
    int8_t col_off = kDirections[dir][1];

    // If the piece can be moved for at least one sqaure, then the move is
    // valid. Here, we call the square being checked that is one square away
    // from the source "the destination" for simplicity, even though the
    // piece should move all the way in that direction until it hits a wall
    // or another piece.
    int8_t dest_row = row + row_off;
    int8_t dest_col = col + col_off;

    // Cannot move outside the board.
    if (!OnBoard(dest_row, dest_col)) return false;

    // Cannot make the move if the direction is blocked by another piece.
    if (board[ToIndex(dest_row, dest_col)] != '-') return false;

    return true;
}

static void SwapPieces(char *a, char *b) {
    char tmp = *a;
    *a = *b;
    *b = tmp;
}

// Moves the piece at index SRC on BOARD in the given DIRection all the way
// until it is blocked by an edge or another piece. Returns the index of the
// destination.
static int8_t MovePiece(char board[static kBoardSize], int8_t src, int8_t dir) {
    int8_t row = src / kBoardCols;
    int8_t col = src % kBoardCols;
    int8_t row_off = kDirections[dir][0];
    int8_t col_off = kDirections[dir][1];
    while (OnBoard(row + row_off, col + col_off) &&
           board[ToIndex(row + row_off, col + col_off)] == '-') {
        row += row_off;
        col += col_off;
    }

    int8_t dest = ToIndex(row, col);
    SwapPieces(&board[src], &board[dest]);

    return dest;
}

static bool PieceMoveAvailable(const char board[static kBoardSize], int turn) {
    char piece_to_move = kTurnToPiece[turn];
    for (int8_t i = 0; i < kBoardSize; ++i) {
        if (board[i] != piece_to_move) continue;
        for (int8_t dir = 0; dir < 8; ++dir) {
            if (CanMoveInDirection(board, i, dir)) {
                return true;
            }
        }
    }

    return false;
}

static void GeneratePieceMoves(const char board[static kBoardSize], int turn,
                               int8_t n_src, int8_t n_dir, MoveArray *moves) {
    char piece_to_move = kTurnToPiece[turn];
    NeutronMove m = kNeutronMoveInit;
    m.unpacked.n_src = n_src;
    m.unpacked.n_dir = n_dir;
    for (int8_t i = 0; i < kBoardSize; ++i) {
        if (board[i] != piece_to_move) continue;
        for (int8_t dir = 0; dir < 8; ++dir) {
            if (CanMoveInDirection(board, i, dir)) {
                m.unpacked.p_src = i;
                m.unpacked.p_dir = dir;
                MoveArrayAppend(moves, m.hashed);
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

static bool IsInHomeRows(int8_t i) {
    return (0 <= i && i < 5) || (20 <= i && i < 25);
}

static bool MoveAvailable(Position pos, char board[static kBoardSize],
                          int turn) {
    if (pos == kInitialPosition) return true;

    int8_t n_src = FindNeutron(board);
    bool found = false;
    // For each possible neutron move
    for (int8_t n_dir = 0; !found && n_dir < 8; ++n_dir) {
        // Make the current neutron move and generate piece moves.
        int8_t dest = MovePiece(board, n_src, n_dir);
        // If cannot move the neutron in this direction, skip it.
        if (dest == n_src) continue;
        found = IsInHomeRows(dest) || PieceMoveAvailable(board, turn);

        // Revert the neutron move. Safe to assume that n_src != dest.
        board[n_src] = 'N';
        board[dest] = '-';
    }

    return found;
}

/** \p board may be modified during the function call, but will be restored
 *  upon returning. */
static MoveArray NeutronGenerateMovesInternal(Position pos,
                                              char board[static kBoardSize],
                                              int turn) {
    MoveArray ret;
    MoveArrayInit(&ret);

    // If the given position is the initial position, generate piece moves
    // directly.
    if (pos == kInitialPosition) {
        GeneratePieceMoves(board, turn, -1, 0, &ret);
        return ret;
    }

    int8_t n_src = FindNeutron(board);

    // For each possible neutron move
    for (int8_t n_dir = 0; n_dir < 8; ++n_dir) {
        // Make the current neutron move and generate piece moves.
        int8_t dest = MovePiece(board, n_src, n_dir);
        // If cannot move the neutron in this direction, skip it.
        if (dest == n_src) continue;

        if (IsInHomeRows(dest)) {  // Game already over after neutron move.
            NeutronMove m = kNeutronMoveInit;
            m.unpacked.n_src = n_src;
            m.unpacked.n_dir = n_dir;
            MoveArrayAppend(&ret, m.hashed);
        } else {
            GeneratePieceMoves(board, turn, n_src, n_dir, &ret);
        }

        // Revert the neutron move. Safe to assume that n_src != dest.
        board[n_src] = 'N';
        board[dest] = '-';
    }

    return ret;
}

static MoveArray NeutronGenerateMoves(Position position) {
    // Unhash
    char board[kBoardSize];
    bool success = Unhash(position, board);
    assert(success);
    (void)success;
    int turn = GetTurn(position);

    return NeutronGenerateMovesInternal(position, board, turn);
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
    if (!MoveAvailable(position, board, turn)) return kLose;

    return kUndecided;
}

static Position NeutronDoMoveInternal(const char board[static kBoardSize],
                                      int turn, Move move) {
    NeutronMove m = {.hashed = move};
    char board_copy[kBoardSize];
    memcpy(board_copy, board, kBoardSize);
    if (m.unpacked.n_src >= 0) {  // Not initial position, perform neutron move.
        MovePiece(board_copy, m.unpacked.n_src, m.unpacked.n_dir);
    }

    // Perform piece move if necessary.
    if (m.unpacked.p_src >= 0) {
        MovePiece(board_copy, m.unpacked.p_src, m.unpacked.p_dir);
    }

    return GenericHashHash(board_copy, 3 - turn);
}

static Position NeutronDoMove(Position position, Move move) {
    // Unhash
    char board[kBoardSize];
    bool success = Unhash(position, board);
    assert(success);
    (void)success;
    int turn = GetTurn(position);

    return NeutronDoMoveInternal(board, turn, move);
}

static bool NeutronIsLegalPosition(Position position) {
    (void)position;  // No simple way to check for illegal positions.
    return true;
}

static Position GetCanonicalPositionInternal(Position position,
                                             const char board[kBoardSize],
                                             int turn) {
    char sym_board[kBoardSize];

    // Apply the only symmetry.
    for (int i = 0; i < kBoardSize; ++i) {
        sym_board[i] = board[kSymmetry[i]];
    }
    Position sym = GenericHashHash(sym_board, turn);

    return position < sym ? position : sym;
}

static Position NeutronGetCanonicalPosition(Position position) {
    // The initial position happens to be a canonical position by the only
    // available reflection symmetry.
    if (position == kInitialPosition) return position;

    // Unhash
    char board[kBoardSize];
    // This call to GenericHashUnhash is safe because we already dealt with the
    // initial position.
    bool success = GenericHashUnhash(position, board);
    assert(success);
    (void)success;
    int turn = GenericHashGetTurn(position);

    return GetCanonicalPositionInternal(position, board, turn);
}

static PositionArray NeutronGetCanonicalChildPositions(Position position) {
    // Unhash
    char board[kBoardSize];
    bool success = Unhash(position, board);
    assert(success);
    (void)success;
    int turn = GetTurn(position);

    // Generate moves
    MoveArray moves = NeutronGenerateMovesInternal(position, board, turn);
    PositionHashSet dedup;
    PositionHashSetInit(&dedup, 0.5);
    PositionArray ret;
    PositionArrayInit(&ret);
    for (int64_t i = 0; i < moves.size; ++i) {
        Position child = NeutronDoMoveInternal(board, turn, moves.array[i]);
        child = NeutronGetCanonicalPosition(child);
        if (!PositionHashSetContains(&dedup, child)) {
            PositionHashSetAdd(&dedup, child);
            PositionArrayAppend(&ret, child);
        }
    }
    PositionHashSetDestroy(&dedup);
    MoveArrayDestroy(&moves);

    return ret;
}

static bool NeutronReachedHomeRows(const char board[static kBoardSize]) {
    for (int i = 0; i < 5; ++i) {
        if (board[kXHomeRow[i]] == 'N' || board[kOHomeRow[i]] == 'N') {
            return true;
        }
    }

    return false;
}

// Returns whether the piece at SRC could have been moved from the given
// DIRection in the previous turn.
static bool CanComeFromDirection(const char board[static kBoardSize], int src,
                                 int dir) {
    int opposite_dir = 7 - dir;
    return !CanMoveInDirection(board, src, opposite_dir);
}

static int8_t ShiftPiece(char board[static kBoardSize], int8_t src,
                         int8_t dir) {
    int8_t row = src / kBoardCols;
    int8_t col = src % kBoardCols;
    int8_t row_off = kDirections[dir][0];
    int8_t col_off = kDirections[dir][1];
    int8_t dest_row = row + row_off;
    int8_t dest_col = col + col_off;

    // Cannot move outside the board.
    if (!OnBoard(dest_row, dest_col)) return src;

    // Cannot make the move if the direction is blocked by another piece.
    int8_t dest = ToIndex(dest_row, dest_col);
    if (board[dest] != '-') return src;

    board[dest] = board[src];
    board[src] = '-';

    return dest;
}

static void GenerateParentsByReversingNeutron(char board[static kBoardSize],
                                              int prev_turn, int8_t i,
                                              PositionArray *ret,
                                              PositionHashSet *dedup) {
    for (int8_t dir = 0; dir < 8; ++dir) {
        if (!CanComeFromDirection(board, i, dir)) continue;
        int8_t prev_dest = i, dest = ShiftPiece(board, prev_dest, dir);
        while (dest != prev_dest) {
            Position parent = GenericHashHash(board, prev_turn);
            parent = GetCanonicalPositionInternal(parent, board, prev_turn);
            if (!PositionHashSetContains(dedup, parent)) {
                PositionHashSetAdd(dedup, parent);
                PositionArrayAppend(ret, parent);
            }
            prev_dest = dest;
            dest = ShiftPiece(board, prev_dest, dir);
        }
        // Revert piece shifting. Note that dest may equal to i.
        SwapPieces(&board[i], &board[dest]);
    }
}

static PositionArray NeutronGetCanonicalParentPositions(Position position) {
    PositionArray ret;
    PositionArrayInit(&ret);

    // The initial position has no parents.
    if (position == kInitialPosition) return ret;

    // Unhash
    char board[kBoardSize];
    bool success = Unhash(position, board);
    assert(success);
    (void)success;
    int prev_turn = 3 - GetTurn(position);
    char piece_moved_prev_turn = kTurnToPiece[prev_turn];
    int8_t neutron_index = FindNeutron(board);
    PositionHashSet dedup;
    PositionHashSetInit(&dedup, 0.5);

    if (NeutronReachedHomeRows(board)) {
        // If the neutron is on one of the two home rows, then the player in
        // the previous turn didn't make a piece move. Only reverse the move of
        // the neutron...
        GenerateParentsByReversingNeutron(board, prev_turn, neutron_index, &ret,
                                          &dedup);
    } else {
        // ... otherwise, first reverse the move of any one of the opponent
        // pieces, then reverse the move of the neutron.
        for (int8_t i = 0; i < kBoardSize; ++i) {
            if (board[i] != piece_moved_prev_turn) continue;
            for (int8_t dir = 0; dir < 8; ++dir) {
                if (!CanComeFromDirection(board, i, dir)) continue;
                int8_t prev_dest = i, dest = ShiftPiece(board, prev_dest, dir);
                while (dest != prev_dest) {
                    GenerateParentsByReversingNeutron(
                        board, prev_turn, neutron_index, &ret, &dedup);
                    prev_dest = dest;
                    dest = ShiftPiece(board, prev_dest, dir);
                }
                // Revert piece shifting. Note that dest may equal to i.
                SwapPieces(&board[i], &board[dest]);
            }
        }
        // If position is reachable from the initial position, also append the
        // initial position to the return array.
        if (PositionHashSetContains(&kChildrenOfInitialPosition, position)) {
            PositionArrayAppend(&ret, kInitialPosition);
        }
    }
    PositionHashSetDestroy(&dedup);

    return ret;
}

static const RegularSolverApi kNeutronSolverApi = {
    .GetNumPositions = NeutronGetNumPositions,
    .GetInitialPosition = NeutronGetInitialPosition,

    .GenerateMoves = NeutronGenerateMoves,
    .Primitive = NeutronPrimitive,
    .DoMove = NeutronDoMove,
    .IsLegalPosition = NeutronIsLegalPosition,
    .GetCanonicalPosition = NeutronGetCanonicalPosition,
    .GetCanonicalChildPositions = NeutronGetCanonicalChildPositions,
    .GetCanonicalParentPositions = NeutronGetCanonicalParentPositions,
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
    NeutronMove m = {.hashed = move};
    if (m.unpacked.n_src < 0) {  // No neutron move
        sprintf(buffer, "%d %s", m.unpacked.p_src + 1,
                kDirectionStr[m.unpacked.p_dir]);
    } else if (m.unpacked.p_src < 0) {  // No piece move
        sprintf(buffer, "%d %s END", m.unpacked.n_src + 1,
                kDirectionStr[m.unpacked.n_dir]);
    } else {
        sprintf(buffer, "%d %s %d %s", m.unpacked.n_src + 1,
                kDirectionStr[m.unpacked.n_dir], m.unpacked.p_src + 1,
                kDirectionStr[m.unpacked.p_dir]);
    }

    return kNoError;
}

static int8_t DirectionStrToDir(const char *dir_str) {
    for (int8_t i = 0; i < 8; ++i) {
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
    // 1. Initial position: "p_src p_dir"
    // 2. Any other position:
    //    a. "n_src n_dir END" if game is over after neutron movement, or
    //    b. "n_src n_dir p_src p_dir" otherwise.
    if (move_string == NULL) return false;
    size_t len = strlen(move_string);
    if (len < 3 || len > 11) return false;

    char move_string_copy[12];
    strcpy(move_string_copy, move_string);
    char *tokens[4], *saveptr;
    tokens[0] = strtok_r(move_string_copy, kMoveStrDelim, &saveptr);
    for (int i = 1; i < 4; ++i) {
        tokens[i] = strtok_r(NULL, kMoveStrDelim, &saveptr);
    }

    // Validate the first two tokens
    if (!IsValidMoveTokenPair(tokens[0], tokens[1])) return false;

    // If no more tokens or game ends after neutron move, the format is valid.
    if (tokens[2] == NULL || strcmp(tokens[2], "END") == 0) {
        return tokens[3] == NULL;
    }
    // Otherwise, also check for the last two tokens.
    if (!IsValidMoveTokenPair(tokens[2], tokens[3])) return false;

    return true;
}

static Move NeutronStringToMove(ReadOnlyString move_string) {
    char move_string_copy[12];
    strcpy(move_string_copy, move_string);
    char *tokens[4], *saveptr;
    tokens[0] = strtok_r(move_string_copy, kMoveStrDelim, &saveptr);
    for (int i = 1; i < 4; ++i) {
        tokens[i] = strtok_r(NULL, kMoveStrDelim, &saveptr);
    }

    NeutronMove m = kNeutronMoveInit;
    if (tokens[2] == NULL) {  // Move at the initial position.
        assert(tokens[3] == NULL);
        m.unpacked.p_src = atoi(tokens[0]) - 1;
        m.unpacked.p_dir = DirectionStrToDir(tokens[1]);
    } else if (strcmp(tokens[2], "END") == 0) {  // Game over after neutron move
        assert(tokens[3] == NULL);
        m.unpacked.n_src = atoi(tokens[0]) - 1;
        m.unpacked.n_dir = DirectionStrToDir(tokens[1]);
    } else {  // All other moves.
        assert(tokens[3] != NULL);
        m.unpacked.n_src = atoi(tokens[0]) - 1;
        m.unpacked.n_dir = DirectionStrToDir(tokens[1]);
        m.unpacked.p_src = atoi(tokens[2]) - 1;
        m.unpacked.p_dir = DirectionStrToDir(tokens[3]);
    }

    return m.hashed;
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

// ========================= kNeutronUwapiRegular =========================

// Formal position format:
// """
// [turn]_[board (25x)]
// OR a single 'I' character for the initial position.
// """
static bool NeutronIsLegalFormalPosition(ReadOnlyString formal_position) {
    int len = strlen(formal_position);
    // Special case for the initial position
    if (len == 1) return toupper(formal_position[0] == 'I');
    if (len != kBoardSize + 2) return false;
    if (formal_position[0] != '1' && formal_position[0] != '2') return false;
    if (formal_position[1] != '_') return false;

    // Count the number of each type of piece.
    int x_count = 0, o_count = 0, n_count = 0;
    for (int i = 2; i < kBoardSize + 2; ++i) {
        char c = toupper(formal_position[i]);
        if (c == 'X') {
            ++x_count;
        } else if (c == 'O') {
            ++o_count;
        } else if (c == 'N') {
            ++n_count;
        } else if (c != '-') {  // Illegal character detected.
            return false;
        }
    }

    if (n_count != 1) return false;  // There must be exactly 1 neutron piece.
    if (x_count != 5 || o_count != 5) return false;  // 5 pieces each.

    return true;
}

static Position NeutronFormalPositionToPosition(
    ReadOnlyString formal_position) {
    // Special case for the initial position.
    if (formal_position[0] == 'I') return kInitialPosition;

    // Hash
    char board[kBoardSize];
    memcpy(board, formal_position + 2, kBoardSize);
    for (int i = 0; i < kBoardSize; ++i) {
        board[i] = toupper(board[i]);
    }
    int turn = formal_position[0] - '0';

    return GenericHashHash(board, turn);
}

static CString NeutronPositionToFormalPosition(Position position) {
    // Special case for the initial position.
    if (position == kInitialPosition) {
        CString ret;
        CStringInitCopyCharArray(&ret, "I");
        return ret;
    }

    // Must be NULL-terminated for AutoGuiMakePosition.
    char board[kBoardSize + 1] = {0};

    // Unhash
    if (!Unhash(position, board)) {
        NotReached(
            "NeutronPositionToFormalPosition: failed to unhash position");
    }
    int turn = GetTurn(position);

    return AutoGuiMakePosition(turn, board);
}

static CString NeutronPositionToAutoGuiPosition(Position position) {
    // Return a hard-coded string for the initial position.
    if (position == kInitialPosition) {
        return AutoGuiMakePosition(1, kInitialBoard);
    }

    // For all other positions, use the same format as formal.
    return NeutronPositionToFormalPosition(position);
}

static ConstantReadOnlyString kBoardIndexToLegend[] = {
    "a5", "b5", "c5", "d5", "e5", "a4", "b4", "c4", "d4",
    "e4", "a3", "b3", "c3", "d3", "e3", "a2", "b2", "c2",
    "d2", "e2", "a1", "b1", "c1", "d1", "e1",
};

static CString NeutronMoveToFormalMove(Position position, Move move) {
    (void)position;  // Unused.

    // The following logic works for both part-moves and full-moves.
    char buffer[12];
    NeutronMove m = {.hashed = move};
    if (m.unpacked.n_src < 0) {  // No neutron move
        assert(m.unpacked.p_src >= 0);
        sprintf(buffer, "%s %s", kBoardIndexToLegend[m.unpacked.p_src],
                kDirectionStr[m.unpacked.p_dir]);
    } else if (m.unpacked.p_src < 0) {  // No piece move
        sprintf(buffer, "N %s", kDirectionStr[m.unpacked.n_dir]);
    } else {
        sprintf(buffer, "N %s %s %s", kDirectionStr[m.unpacked.n_dir],
                kBoardIndexToLegend[m.unpacked.p_src],
                kDirectionStr[m.unpacked.p_dir]);
    }
    CString ret;
    CStringInitCopyCharArray(&ret, buffer);

    return ret;
}

static int GetMoveDestination(const char board[kBoardSize], int8_t src,
                              int8_t dir) {
    int8_t row = src / kBoardCols;
    int8_t col = src % kBoardCols;
    int8_t row_off = kDirections[dir][0];
    int8_t col_off = kDirections[dir][1];
    while (OnBoard(row + row_off, col + col_off) &&
           board[ToIndex(row + row_off, col + col_off)] == '-') {
        row += row_off;
        col += col_off;
    }

    return ToIndex(row, col);
}

static CString NeutronMoveToAutoGuiMove(Position position, Move move) {
    static const char kSoundChar = 'x';

    // Unhash
    char board[kBoardSize];
    if (!Unhash(position, board)) return kNullCString;
    NeutronMove m = {.hashed = move};
    if (m.unpacked.n_src < 0) {  // No neutron move.
        return AutoGuiMakeMoveM(
            m.unpacked.p_src,
            GetMoveDestination(board, m.unpacked.p_src, m.unpacked.p_dir),
            kSoundChar);
    } else if (m.unpacked.p_src < 0) {  // No piece move.
        return AutoGuiMakeMoveM(
            m.unpacked.n_src,
            GetMoveDestination(board, m.unpacked.n_src, m.unpacked.n_dir),
            kSoundChar);
    } else {  // A full multipart move does not have an AutoGUI string.
        return kNullCString;
    }

    return kNullCString;
}

static CString AddNeutronPartmove(Position pos,
                                  const char board[kBoardSize + 1], int turn,
                                  int8_t n_src, int8_t n_dir,
                                  PartmoveArray *partmoves) {
    NeutronMove m = kNeutronMoveInit;
    m.unpacked.n_src = n_src;
    m.unpacked.n_dir = n_dir;
    CString autogui_move = NeutronMoveToAutoGuiMove(pos, m.hashed);
    CString formal_move = NeutronMoveToFormalMove(pos, m.hashed);
    CString to = AutoGuiMakePosition(turn, board);
    CString to_copy;
    CStringInitCopy(&to_copy, &to);
    PartmoveArrayEmplaceBack(partmoves, &autogui_move, &formal_move, NULL,
                             &to_copy, NULL);

    return to;
}

static void AddPiecePartmove(Position pos, const CString *from, int8_t n_src,
                             int8_t n_dir, int8_t p_src, int8_t p_dir,
                             PartmoveArray *partmoves) {
    NeutronMove m = kNeutronMoveInit;
    m.unpacked.p_src = p_src;
    m.unpacked.p_dir = p_dir;
    CString autogui_move = NeutronMoveToAutoGuiMove(pos, m.hashed);
    CString formal_move = NeutronMoveToFormalMove(pos, m.hashed);

    m.unpacked.n_src = n_src;
    m.unpacked.n_dir = n_dir;
    CString full = NeutronMoveToFormalMove(pos, m.hashed);
    CString from_copy;
    CStringInitCopy(&from_copy, from);
    PartmoveArrayEmplaceBack(partmoves, &autogui_move, &formal_move, &from_copy,
                             NULL, &full);
}

static void GeneratePiecePartmoves(Position pos,
                                   const char board[static kBoardSize + 1],
                                   int turn, const CString *from, int8_t n_src,
                                   int8_t n_dir, PartmoveArray *partmoves) {
    char piece_to_move = kTurnToPiece[turn];
    for (int8_t i = 0; i < kBoardSize; ++i) {
        if (board[i] != piece_to_move) continue;
        for (int8_t dir = 0; dir < 8; ++dir) {
            if (CanMoveInDirection(board, i, dir)) {
                AddPiecePartmove(pos, from, n_src, n_dir, i, dir, partmoves);
            }
        }
    }
}

static PartmoveArray NeutronGeneratePartmovesInternal(
    Position pos, char board[static kBoardSize + 1], int turn) {
    //
    PartmoveArray ret;
    PartmoveArrayInit(&ret);

    // If the given position is the initial position, then all moves are full
    // single-part moves and they are not handled in this function.
    if (pos == kInitialPosition) return ret;

    int8_t n_src = FindNeutron(board);
    // For each possible neutron move
    for (int8_t n_dir = 0; n_dir < 8; ++n_dir) {
        // Make the current neutron move and generate piece moves.
        int8_t dest = MovePiece(board, n_src, n_dir);

        // If cannot move the neutron in this direction, skip it.
        if (dest == n_src) continue;

        // If game is already over after the current neutron move, then the move
        // is also a full move, which should be skipped by this function.
        if (!IsInHomeRows(dest)) {
            CString intermediate =
                AddNeutronPartmove(pos, board, turn, n_src, n_dir, &ret);
            GeneratePiecePartmoves(GenericHashHash(board, turn), board, turn,
                                   &intermediate, n_src, n_dir, &ret);
            CStringDestroy(&intermediate);
        }

        // Revert the neutron move. Safe to assume that n_src != dest.
        board[n_src] = 'N';
        board[dest] = '-';
    }

    return ret;
}

static PartmoveArray NeutronGeneratePartmoves(Position position) {
    // Unhash
    char board[kBoardSize + 1] = {0};
    Unhash(position, board);
    int turn = GetTurn(position);

    return NeutronGeneratePartmovesInternal(position, board, turn);
}

static const UwapiRegular kNeutronUwapiRegular = {
    .GenerateMoves = NeutronGenerateMoves,
    .DoMove = NeutronDoMove,
    .Primitive = NeutronPrimitive,

    .IsLegalFormalPosition = NeutronIsLegalFormalPosition,
    .FormalPositionToPosition = NeutronFormalPositionToPosition,
    .PositionToFormalPosition = NeutronPositionToFormalPosition,
    .PositionToAutoGuiPosition = NeutronPositionToAutoGuiPosition,
    .MoveToFormalMove = NeutronMoveToFormalMove,
    .MoveToAutoGuiMove = NeutronMoveToAutoGuiMove,
    .GetInitialPosition = NeutronGetInitialPosition,
    .GeneratePartmoves = NeutronGeneratePartmoves,
    .GetRandomLegalPosition = NULL,  // Not available for this game.
};

static const Uwapi kNeutronUwapi = {.regular = &kNeutronUwapiRegular};

// ================================ NeutronInit ================================

static int NeutronInit(void *aux) {
    (void)aux;  // Unused.
    GenericHashReinitialize();

    // Using X for the first player's pieces, O for the second player's pieces,
    // and N for the neutron.
    int pieces_init[] = {'X', 5, 5, 'O', 5, 5, 'N', 1, 1, '-', 14, 14, -1};
    GenericHashAddContext(0, kBoardSize, pieces_init, NULL, 0);
    kInitialPosition = GenericHashNumPositions();

    // Cache the children of the initial position.
    PositionHashSetInit(&kChildrenOfInitialPosition, 0.5);
    MoveArray moves = NeutronGenerateMoves(kInitialPosition);
    for (int64_t i = 0; i < moves.size; ++i) {
        Position child = NeutronDoMove(kInitialPosition, moves.array[i]);
        PositionHashSetAdd(&kChildrenOfInitialPosition, child);
    }
    MoveArrayDestroy(&moves);

    return kNoError;
}

// ============================== NeutronFinalize ==============================

static int NeutronFinalize(void) {
    PositionHashSetDestroy(&kChildrenOfInitialPosition);
    return kNoError;
}

// ================================= kNeutron =================================

/** @brief Neutron */
const Game kNeutron = {
    .name = "neutron",
    .formal_name = "Neutron",
    .solver = &kRegularSolver,
    .solver_api = &kNeutronSolverApi,
    .gameplay_api = &kNeutronGameplayApi,
    .uwapi = &kNeutronUwapi,

    .Init = NeutronInit,
    .Finalize = NeutronFinalize,

    .GetCurrentVariant = NULL,  // No other variants for now
    .SetVariantOption = NULL,   // No other variants for now
};

// NOLINTEND(cppcoreguidelines-narrowing-conversions)

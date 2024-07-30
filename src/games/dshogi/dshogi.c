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
#include "games/dshogi/dshogi_constants.h"

// ========================== kDobutsuShogiSolverApi ==========================

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
};

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
    .uwapi = NULL,  // TODO

    .Init = DobutsuShogiInit,
    .Finalize = DobutsuShogiFinalize,

    .GetCurrentVariant = NULL,  // No other variants
    .SetVariantOption = NULL,   // No other variants
};

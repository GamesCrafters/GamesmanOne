/**
 * @file mallqueenschess.c
 * @author Andrew Esteban: wrote the original version (mallqueenschess.c in
 * GamesmanClassic.)
 * @author Cameron Cheung (cameroncheung@berkeley.edu): adapted to the new
 * system. GamesCrafters Research Group, UC Berkeley Supervised by Dan Garcia
 * <ddgarcia@cs.berkeley.edu>
 * @brief All Queens Chess implementation
 * @version 1.0.3
 * @date 2024-10-06
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

#include "games/mallqueenschess/mallqueenschess.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // atoi

#include "core/generic_hash/generic_hash.h"
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/types/gamesman_types.h"

// Game, Solver, and Gameplay API Functions

enum {
    boardSize = 25,
    sideLength = 5,
    W = 'W',
    B = 'B',
    BLANK = '-',
};

#define MOVE_ENCODE(from, to) ((from << 5) | to)

static int MallqueenschessInit(void *aux);
static int MallqueenschessFinalize(void);

static const GameVariant *MallqueenschessGetCurrentVariant(void);
static int MallqueenschessSetVariantOption(int option, int selection);

static int64_t MallqueenschessGetNumPositions(void);
static Position MallqueenschessGetInitialPosition(void);

static MoveArray MallqueenschessGenerateMoves(Position position);
static Value MallqueenschessPrimitive(Position position);
static Position MallqueenschessDoMove(Position position, Move move);
static bool MallqueenschessIsLegalPosition(Position position);
static Position MallqueenschessGetCanonicalPosition(Position position);
static PositionArray MallqueenschessGetCanonicalParentPositions(
    Position position);

static int MallqueenschessPositionToString(Position position, char *buffer);
static int MallqueenschessMoveToString(Move move, char *buffer);
static bool MallqueenschessIsValidMoveString(ReadOnlyString move_string);
static Move MallqueenschessStringToMove(ReadOnlyString move_string);

// Solver API Setup
static const RegularSolverApi kSolverApi = {
    .GetNumPositions = &MallqueenschessGetNumPositions,
    .GetInitialPosition = &MallqueenschessGetInitialPosition,
    .GenerateMoves = &MallqueenschessGenerateMoves,
    .Primitive = &MallqueenschessPrimitive,
    .DoMove = &MallqueenschessDoMove,
    .IsLegalPosition = &MallqueenschessIsLegalPosition,
    .GetCanonicalPosition = &MallqueenschessGetCanonicalPosition,
    .GetNumberOfCanonicalChildPositions = NULL,
    .GetCanonicalChildPositions = NULL,
    .GetCanonicalParentPositions = &MallqueenschessGetCanonicalParentPositions,
};

// Gameplay API Setup

static const GameplayApiCommon kGamePlayApiCommon = {
    .GetInitialPosition = &MallqueenschessGetInitialPosition,
    .position_string_length_max = 120,

    .move_string_length_max = 5,
    .MoveToString = &MallqueenschessMoveToString,
    .IsValidMoveString = &MallqueenschessIsValidMoveString,
    .StringToMove = &MallqueenschessStringToMove,
};

static const GameplayApiRegular kGameplayApiRegular = {
    .PositionToString = &MallqueenschessPositionToString,
    .GenerateMoves = &MallqueenschessGenerateMoves,
    .DoMove = &MallqueenschessDoMove,
    .Primitive = &MallqueenschessPrimitive,
};

static const GameplayApi kGameplayApi = {
    .common = &kGamePlayApiCommon,
    .regular = &kGameplayApiRegular,
};

const Game kMallqueenschess = {
    .name = "mallqueenschess",
    .formal_name = "All Queens Chess",
    .solver = &kRegularSolver,
    .solver_api = (const void *)&kSolverApi,
    .gameplay_api = &kGameplayApi,

    .Init = &MallqueenschessInit,
    .Finalize = &MallqueenschessFinalize,

    .GetCurrentVariant = &MallqueenschessGetCurrentVariant,
    .SetVariantOption = &MallqueenschessSetVariantOption,
};

// Helper Types and Global Variables

static const int totalNumBoardSymmetries = 8;
static const int symmetries[8][25] = {
    {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
     13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24},
    {4,  3,  2,  1,  0,  9,  8,  7,  6,  5,  14, 13, 12,
     11, 10, 19, 18, 17, 16, 15, 24, 23, 22, 21, 20},
    {20, 15, 10, 5,  0,  21, 16, 11, 6,  1,  22, 17, 12,
     7,  2,  23, 18, 13, 8,  3,  24, 19, 14, 9,  4},
    {0,  5,  10, 15, 20, 1,  6,  11, 16, 21, 2,  7, 12,
     17, 22, 3,  8,  13, 18, 23, 4,  9,  14, 19, 24},
    {24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,
     11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1,  0},
    {20, 21, 22, 23, 24, 15, 16, 17, 18, 19, 10, 11, 12,
     13, 14, 5,  6,  7,  8,  9,  0,  1,  2,  3,  4},
    {4,  9,  14, 19, 24, 3,  8,  13, 18, 23, 2,  7, 12,
     17, 22, 1,  6,  11, 16, 21, 0,  5,  10, 15, 20},
    {24, 19, 14, 9,  4,  23, 18, 13, 8,  3,  22, 17, 12,
     7,  2,  21, 16, 11, 6,  1,  20, 15, 10, 5,  0}};

// Helper Functions

static void UnhashMove(Move move, int *from, int *to);

static int MallqueenschessInit(void *aux) {
    (void)aux;  // Unused.

    GenericHashReinitialize();
    int pieces_init_array[10] = {BLANK, 13, 13, B, 6, 6, W, 6, 6, -1};
    bool success =
        GenericHashAddContext(0, boardSize, pieces_init_array, NULL, 0);
    if (!success) {
        fprintf(stderr,
                "Mallqueenschess: failed to initialize generic hash context. "
                "Aborting...\n");
        GenericHashReinitialize();
        return kRuntimeError;
    }
    return kNoError;
}

static int MallqueenschessFinalize(void) { return kNoError; }

static const GameVariant *MallqueenschessGetCurrentVariant(void) {
    return NULL;  // Not implemented.
}

static int MallqueenschessSetVariantOption(int option, int selection) {
    (void)option;
    (void)selection;
    return 0;  // Not implemented.
}

// Assumes Generic Hash has been initialized.
static Position MallqueenschessGetInitialPosition(void) {
    return GenericHashHash("WBWBW-----B---W-----BWBWB", 1);
}

static int64_t MallqueenschessGetNumPositions(void) {
    return GenericHashNumPositions();
}

static MoveArray MallqueenschessGenerateMoves(Position position) {
    MoveArray moves;
    MoveArrayInit(&moves);

    char board[boardSize];
    GenericHashUnhash(position, board);

    char turn = GenericHashGetTurn(position) == 1 ? W : B;

    for (int i = 0; i < boardSize; i++) {
        if ((turn == W && board[i] == W) || (turn == B && board[i] == B)) {
            int originRow = i / sideLength;
            int originCol = i % sideLength;
            int origin = i;

            // Left
            for (int col = originCol - 1; col >= 0; col--) {
                if (board[originRow * sideLength + col] == BLANK) {
                    int targetRow = originRow;
                    int targetCol = col;

                    int target = targetRow * sideLength + targetCol;

                    MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));
                } else {
                    break;
                }
            }

            // Right
            for (int col = originCol + 1; col < sideLength; col++) {
                if (board[originRow * sideLength + col] == BLANK) {
                    int targetRow = originRow;
                    int targetCol = col;

                    int target = targetRow * sideLength + targetCol;

                    MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));
                } else {
                    break;
                }
            }

            // Up
            for (int row = originRow - 1; row >= 0; row--) {
                if (board[row * sideLength + originCol] == BLANK) {
                    int targetRow = row;
                    int targetCol = originCol;

                    int target = targetRow * sideLength + targetCol;

                    MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));
                } else {
                    break;
                }
            }

            // Down
            for (int row = originRow + 1; row < sideLength; row++) {
                if (board[row * sideLength + originCol] == BLANK) {
                    int targetRow = row;
                    int targetCol = originCol;

                    int target = targetRow * sideLength + targetCol;

                    MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));
                } else {
                    break;
                }
            }

            // Left-Up
            if (originRow > 0 && originCol > 0) {
                int row = originRow - 1;
                int col = originCol - 1;

                while (row >= 0 && col >= 0) {
                    if (board[row * sideLength + col] == BLANK) {
                        int target = row * sideLength + col;
                        MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));

                        row--;
                        col--;
                    } else {
                        break;
                    }
                }
            }

            // Left-Down
            if (originRow < sideLength - 1 && originCol > 0) {
                int row = originRow + 1;
                int col = originCol - 1;

                while (row < sideLength && col >= 0) {
                    if (board[row * sideLength + col] == BLANK) {
                        int target = row * sideLength + col;
                        MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));

                        row++;
                        col--;
                    } else {
                        break;
                    }
                }
            }

            // Right-Up
            if (originRow > 0 && originCol < sideLength) {
                int row = originRow - 1;
                int col = originCol + 1;

                while (row >= 0 && col < sideLength) {
                    if (board[row * sideLength + col] == BLANK) {
                        int target = row * sideLength + col;
                        MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));

                        row--;
                        col++;
                    } else {
                        break;
                    }
                }
            }

            // Right-Down
            if (originRow < sideLength && originCol < sideLength) {
                int row = originRow + 1;
                int col = originCol + 1;

                while (row < sideLength && col < sideLength) {
                    if (board[row * sideLength + col] == BLANK) {
                        int target = row * sideLength + col;
                        MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));

                        row++;
                        col++;
                    } else {
                        break;
                    }
                }
            }
        }
    }

    return moves;
}

static Value MallqueenschessPrimitive(Position position) {
    char board[boardSize];
    GenericHashUnhash(position, board);

    char piece;

    // Vertical
    int i = 0;
    for (i = 10; i < 15; i++) {
        piece = board[i];
        if (piece != BLANK) {
            if (board[i - 5] == piece && board[i + 5] == piece) {
                if (board[i - 10] == piece || board[i + 10] == piece) {
                    return kLose;
                }
            }
        }
    }

    // Horizontal
    for (i = 2; i < 25; i += 5) {
        piece = board[i];
        if (piece != BLANK) {
            if (board[i - 1] == piece && board[i + 1] == piece) {
                if (board[i - 2] == piece || board[i + 2] == piece) {
                    return kLose;
                }
            }
        }
    }

    piece = board[12];
    if (piece != BLANK) {
        // Antidiagonal
        if (board[6] == piece && board[18] == piece) {
            if (board[0] == piece || board[24] == piece) {
                return kLose;
            }
        }
        // Maindiagonal
        if (board[8] == piece && board[16] == piece) {
            if (board[4] == piece || board[20] == piece) {
                return kLose;
            }
        }
    }

    piece = board[1];
    if (piece != BLANK && board[7] == piece && board[13] == piece &&
        board[19] == piece) {
        return kLose;
    }

    piece = board[5];
    if (piece != BLANK && board[11] == piece && board[17] == piece &&
        board[23] == piece) {
        return kLose;
    }

    piece = board[3];
    if (piece != BLANK && board[7] == piece && board[11] == piece &&
        board[15] == piece) {
        return kLose;
    }

    piece = board[9];
    if (piece != BLANK && board[13] == piece && board[17] == piece &&
        board[21] == piece) {
        return kLose;
    }
    return kUndecided;
}

static Position MallqueenschessDoMove(Position position, Move move) {
    char board[boardSize];
    GenericHashUnhash(position, board);

    int from, to;
    UnhashMove(move, &from, &to);

    board[to] = board[from];
    board[from] = BLANK;

    int oppTurn = GenericHashGetTurn(position) == 1 ? 2 : 1;
    return GenericHashHash(board, oppTurn);
}

static bool MallqueenschessIsLegalPosition(Position position) {
    // Don't need to implement.
    (void)position;
    return true;
}

static Position MallqueenschessGetCanonicalPosition(Position position) {
    char board[boardSize];
    GenericHashUnhash(position, board);

    char pieceInSymmetry, pieceInCurrentCanonical;
    int i, symmetryNum;
    int turn = GenericHashGetTurn(position);

    /* Figure out which symmetry transformation on the input board
    leads to the smallest-ternary-number board in the input board's orbit
    (where the transformations are just rotation/reflection. */
    int bestSymmetryNum = 0;
    for (symmetryNum = 1; symmetryNum < totalNumBoardSymmetries;
         symmetryNum++) {
        for (i = boardSize - 1; i >= 0; i--) {
            pieceInSymmetry = board[symmetries[symmetryNum][i]];
            pieceInCurrentCanonical = board[symmetries[bestSymmetryNum][i]];
            if (pieceInSymmetry != pieceInCurrentCanonical) {
                if (pieceInSymmetry < pieceInCurrentCanonical) {
                    bestSymmetryNum = symmetryNum;
                }
                break;
            }
        }
    }
    char canonBoard[boardSize];
    for (i = 0; i < boardSize; i++) {  // Transform the rest of the board.
        canonBoard[i] = board[symmetries[bestSymmetryNum][i]];
    }

    // Invert the piece colors of the board
    for (i = 0; i < boardSize; i++) {
        if (board[i] == W) {
            board[i] = B;
        } else if (board[i] == B) {
            board[i] = W;
        }
    }

    /* Figure out which symmetry transformation on the swapped input board
    leads to the smallest-ternary-number board in the input board's orbit
    (where the transformations are just rotation/reflection. */
    bestSymmetryNum = 0;
    for (symmetryNum = 1; symmetryNum < totalNumBoardSymmetries;
         symmetryNum++) {
        for (i = boardSize - 1; i >= 0; i--) {
            pieceInSymmetry = board[symmetries[symmetryNum][i]];
            pieceInCurrentCanonical = board[symmetries[bestSymmetryNum][i]];
            if (pieceInSymmetry != pieceInCurrentCanonical) {
                if (pieceInSymmetry < pieceInCurrentCanonical) {
                    bestSymmetryNum = symmetryNum;
                }
                break;
            }
        }
    }
    char canonSwappedBoard[boardSize] = {0};
    for (i = 0; i < boardSize; i++) {  // Transform the rest of the board.
        canonSwappedBoard[i] = board[symmetries[bestSymmetryNum][i]];
    }

    // Compare canonBoard and canonSwappedBoard
    char pieceInRegular, pieceInSwapped;
    for (i = boardSize - 1; i >= 0; i--) {
        pieceInRegular = canonBoard[i];
        pieceInSwapped = canonSwappedBoard[i];
        if (pieceInRegular < pieceInSwapped) {
            return GenericHashHash(canonBoard, turn);
        } else if (pieceInSwapped < pieceInRegular) {
            return GenericHashHash(canonSwappedBoard, turn == 1 ? 2 : 1);
        }
    }
    return GenericHashHash(canonBoard, 1);
}

static PositionArray MallqueenschessGetCanonicalParentPositions(
    Position position) {
    /* The parent positions can be found by swapping the turn of
    the position to get position P', getting the children of
    P', canonicalizing them, then swapping the turn of each
    of those canonical children. */

    char board[boardSize];
    GenericHashUnhash(position, board);
    int t = GenericHashGetTurn(position);
    int oppT = t == 1 ? 2 : 1;
    Position turnSwappedPos = GenericHashHash(board, oppT);

    PositionHashSet deduplication_set;
    PositionHashSetInit(&deduplication_set, 0.5);

    PositionArray canonicalParents;
    PositionArrayInit(&canonicalParents);

    Position child;
    MoveArray moves = MallqueenschessGenerateMoves(turnSwappedPos);
    for (int64_t i = 0; i < moves.size; i++) {
        child = MallqueenschessDoMove(turnSwappedPos, moves.array[i]);
        /* Note that at this point, it is current player's turn at `child`.
        We check if it's primitive before we swap the turn because
        primitive doesn't care about turn. */
        if (MallqueenschessPrimitive(child) == kUndecided) {
            GenericHashUnhash(child, board);
            child = GenericHashHash(board, oppT);  // Now it's opponent's turn
            child = MallqueenschessGetCanonicalPosition(child);
            if (!PositionHashSetContains(&deduplication_set, child)) {
                PositionHashSetAdd(&deduplication_set, child);
                PositionArrayAppend(&canonicalParents, child);
            }
        }
    }
    PositionHashSetDestroy(&deduplication_set);
    MoveArrayDestroy(&moves);
    return canonicalParents;
}

static int MallqueenschessPositionToString(Position position, char *buffer) {
    char board[boardSize];
    GenericHashUnhash(position, board);
    char turn = GenericHashGetTurn(position) == 1 ? W : B;

    static ConstantReadOnlyString kFormat =
        "\n"
        "1 %c%c%c%c%c\n"
        "2 %c%c%c%c%c\n"
        "3 %c%c%c%c%c\n"
        "4 %c%c%c%c%c\n"
        "5 %c%c%c%c%c\n"
        "  abcde          TURN: %c\n";

    int actual_length = snprintf(
        buffer, kGamePlayApiCommon.position_string_length_max + 1, kFormat,
        board[0], board[1], board[2], board[3], board[4], board[5], board[6],
        board[7], board[8], board[9], board[10], board[11], board[12],
        board[13], board[14], board[15], board[16], board[17], board[18],
        board[19], board[20], board[21], board[22], board[23], board[24], turn);

    if (actual_length >= kGamePlayApiCommon.position_string_length_max + 1) {
        fprintf(stderr,
                "MallqueenschessTierPositionToString: (BUG) not enough space "
                "was allocated "
                "to buffer. Please increase position_string_length_max.\n");
        return 1;
    }
    return 0;
}

static int MallqueenschessMoveToString(Move move, char *buffer) {
    int from, to;
    UnhashMove(move, &from, &to);
    int fromRow = from / sideLength;
    int fromCol = from % sideLength;
    int toRow = to / sideLength;
    int toCol = to % sideLength;

    int actual_length = snprintf(
        buffer, kGamePlayApiCommon.move_string_length_max + 1, "%d%c%d%c",
        fromRow + 1, fromCol + 'a', toRow + 1, toCol + 'a');
    if (actual_length >= kGamePlayApiCommon.move_string_length_max + 1) {
        fprintf(
            stderr,
            "MallqueenschessMoveToString: (BUG) not enough space was allocated "
            "to buffer. Please increase move_string_length_max.\n");
        return 1;
    }
    return 0;
}

static bool MallqueenschessIsValidMoveString(ReadOnlyString move_string) {
    if (move_string[0] < '1' || move_string[0] > '5') {
        return false;
    } else if (move_string[1] < 'a' || move_string[1] > 'e') {
        return false;
    } else if (move_string[2] < '1' || move_string[2] > '5') {
        return false;
    } else if (move_string[3] < 'a' || move_string[3] > 'e') {
        return false;
    }
    return true;
}

static Move MallqueenschessStringToMove(ReadOnlyString move_string) {
    assert(MallqueenschessIsValidMoveString(move_string));

    int fromRow = move_string[0] - '1';
    int fromCol = move_string[1] - 'a';
    int from = fromRow * sideLength + fromCol;

    int toRow = move_string[2] - '1';
    int toCol = move_string[3] - 'a';
    int to = toRow * sideLength + toCol;

    return MOVE_ENCODE(from, to);
}

static void UnhashMove(Move move, int *from, int *to) {
    *from = (int)(move >> 5);
    *to = (int)(move & 0x1F);
}

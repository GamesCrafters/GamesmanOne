/**
 * @file kaooa.c
 * @author  Xiang Zheng
            Sriya Kantipudi
            Maria Rufova
            Benji Xu
            Robert Shi
            UC Berkeley Supervised by Dan Garcia
 * <ddgarcia@cs.berkeley.edu>
 * @brief Kaooa implementation
 * @version 1.0.0
 * @date 2024-Oct-18
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

#include "games/kaooa/kaooa.h"

#include <assert.h>    // assert
#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t
#include <stdio.h>     // fprintf, stderr
#include <stdlib.h>    // atoi

#include "core/generic_hash/generic_hash.h"
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/types/gamesman_types.h"

// Game, Solver, and Gameplay API Functions

#define boardSize 10
#define sideLength 5 // ignore
#define MOVE_ENCODE(from, to) ((from << 5) | to) // TODO
#define V 'V'
#define C 'C'
#define BLANK '-'

static int MkaooaInit(void *aux);
static int MkaooaFinalize(void);

static const GameVariant *MkaooaGetCurrentVariant(void);
static int MkaooaSetVariantOption(int option, int selection);

static int64_t MkaooaGetNumPositions(void);
static Position MkaooaGetInitialPosition(void);

static MoveArray MkaooaGenerateMoves(Position position);
static Value MkaooaPrimitive(Position position);
static Position MkaooaDoMove(Position position, Move move);
static bool MkaooaIsLegalPosition(Position position);
static Position MkaooaGetCanonicalPosition(Position position);
static PositionArray MkaooaGetCanonicalParentPositions(
    Position position);

static int MkaooaPositionToString(Position position, char *buffer);
static int MkaooaMoveToString(Move move, char *buffer);
static bool MkaooaIsValidMoveString(ReadOnlyString move_string);
static Move MkaooaStringToMove(ReadOnlyString move_string);

// Solver API Setup
static const RegularSolverApi kSolverApi = {
    .GetNumPositions = &MkaooaGetNumPositions,
    .GetInitialPosition = &MkaooaGetInitialPosition,
    .GenerateMoves = &MkaooaGenerateMoves,
    .Primitive = &MkaooaPrimitive,
    .DoMove = &MkaooaDoMove,
    .IsLegalPosition = &MkaooaIsLegalPosition,
    .GetCanonicalPosition = &MkaooaGetCanonicalPosition,
    .GetNumberOfCanonicalChildPositions = NULL,
    .GetCanonicalChildPositions = NULL,
    .GetCanonicalParentPositions = &MkaooaGetCanonicalParentPositions,
};

// Gameplay API Setup
static const GameplayApiCommon kGamePlayApiCommon = {
    .GetInitialPosition = &MkaooaGetInitialPosition,
    .position_string_length_max = 120, // TODO 

    .move_string_length_max = 5, //TODO
    .MoveToString = &MkaooaMoveToString,
    .IsValidMoveString = &MkaooaIsValidMoveString,
    .StringToMove = &MkaooaStringToMove,
};

static const GameplayApiRegular kGameplayApiRegular = {
    .PositionToString = &MkaooaPositionToString,
    .GenerateMoves = &MkaooaGenerateMoves,
    .DoMove = &MkaooaDoMove,
    .Primitive = &MkaooaPrimitive,
};

static const GameplayApi kGameplayApi = {
    .common = &kGamePlayApiCommon,
    .regular = &kGameplayApiRegular,
};


// Game Definition Setup
const Game kMkaooa = {
    .name = "mkaooa",
    .formal_name = "Kaooa",
    .solver = &kRegularSolver,
    .solver_api = (const void *)&kSolverApi,
    .gameplay_api = (const GameplayApi *)&kGameplayApi,

    .Init = &MkaooaInit,
    .Finalize = &MkaooaFinalize,

    .GetCurrentVariant = &MkaooaGetCurrentVariant,
    .SetVariantOption = &MkaooaSetVariantOption,
};

// Helper Types and Global Variables

//TODO for FUTUREEEEE: Symmetries (not essential rn)

// static const int totalNumBoardSymmetries = 8;
// static const int symmetries[8][25] = {
//     {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
//      13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24},
//     {4,  3,  2,  1,  0,  9,  8,  7,  6,  5,  14, 13, 12,
//      11, 10, 19, 18, 17, 16, 15, 24, 23, 22, 21, 20},
//     {20, 15, 10, 5,  0,  21, 16, 11, 6,  1,  22, 17, 12,
//      7,  2,  23, 18, 13, 8,  3,  24, 19, 14, 9,  4},
//     {0,  5,  10, 15, 20, 1,  6,  11, 16, 21, 2,  7, 12,
//      17, 22, 3,  8,  13, 18, 23, 4,  9,  14, 19, 24},
//     {24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,
//      11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1,  0},
//     {20, 21, 22, 23, 24, 15, 16, 17, 18, 19, 10, 11, 12,
//      13, 14, 5,  6,  7,  8,  9,  0,  1,  2,  3,  4},
//     {4,  9,  14, 19, 24, 3,  8,  13, 18, 23, 2,  7, 12,
//      17, 22, 1,  6,  11, 16, 21, 0,  5,  10, 15, 20},
//     {24, 19, 14, 9,  4,  23, 18, 13, 8,  3,  22, 17, 12,
//      7,  2,  21, 16, 11, 6,  1,  20, 15, 10, 5,  0}};

// Helper Functions

static void UnhashMove(Move move, int *from, int *to);

static int MkaooaInit(void *aux) { // TODO: Need to implement
    (void)aux;  // Unused.

    GenericHashReinitialize();

    // piece, curr # of that piece, max # of that piece
    int pieces_init_array[10] = {BLANK, 10, 10, V, 0, 1, C, 0, 6, -1};

    bool success =
        GenericHashAddContext(0, boardSize, pieces_init_array, NULL, 0);
    if (!success) {
        fprintf(stderr,
                "Mkaooa: failed to initialize generic hash context. "
                "Aborting...\n");
        GenericHashReinitialize();
        return kRuntimeError;
    }
    return kNoError;
}

static int MkaooaFinalize(void) { return kNoError; }

static const GameVariant *MkaooaGetCurrentVariant(void) {
    return NULL;  // Not implemented.
}

static int MkaooaSetVariantOption(int option, int selection) {
    (void)option;
    (void)selection;
    return 0;  // Not implemented.
}

// TODO: Hash the initial blank position of the board

// Assumes Generic Hash has been initialized.
static Position MkaooaGetInitialPosition(void) {  
    return GenericHashHash("WBWBW-----B---W-----BWBWB", 1);
}

static int64_t MkaooaGetNumPositions(void) {
    return GenericHashNumPositions();
}

// (C)row is Player 1 (goes first)
// (V)ulture is Player 2


// TODO: Given a position, generate all the moves available to you from it. 
// The Position will tell you if you are in Phase 1 (dropping 6 crows) or Phase 2 (all pieces on board). 
// You will need to delete a large chunk of the code below. They are olny an example a
static MoveArray MkaooaGenerateMoves(Position position) {
    MoveArray moves;
    MoveArrayInit(&moves);

    char board[boardSize];
    GenericHashUnhash(position, board);

    char turn = GenericHashGetTurn(position) == 1 ? C : V;

    // NOTE: An Example of how moves were generated for All Queen's Chess.
    // Given a game piece, this finds its possible moves in all directions. 
    // Hint: Yours will be a lot shorter
    // You will want to generate different moves according to which phase you are in

    for (int i = 0; i < boardSize; i++) {
        if ((turn == C && board[i] == C) || (turn == V && board[i] == V)) {
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

            //Left-Down, Right-Up, Right-Down removed
        }
    }

    return moves;
}

// TODO: Primitive board position that signals end of game.
// Hint: What is the losing condition? (At what point do we know that a game is basically over?)
// kLose signifies that the current player is losing. In this case, yuo actually do not need to figrue out which player's turn it is, 
// because this function (in the case of our game) should only return kLose or kUndecided. 
static Value MkaooaPrimitive(Position position) {
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
    return kUndecided;
}

//TODO: Figure out how each opponent makes a moves
// Move is a 2-digit num ab, where 'a' is 'from' and 'b' is 'to' value. If a == b, perform drop. Else...
// Hint: We provided you with the psuedo code in slack! 
static Position MkaooaDoMove(Position position, Move move) {
    char board[boardSize];
    GenericHashUnhash(position, board); 

    int from, to;
    UnhashMove(move, &from, &to);

    board[to] = board[from];
    board[from] = BLANK;

    int oppTurn = GenericHashGetTurn(position) == 1 ? 2 : 1;
    return GenericHashHash(board, oppTurn);
}

// NOTE: We think we only need this for optimization, but we will discuss w/ Robert/Dan. 
// Can leave blank for now.
static bool MkaooaIsLegalPosition(Position position) {
    // Don't need to implement.
    (void)position;
    return true;
}


//NOTE: Needed for symmetries, don't worry for no

// static Position MkaooaGetCanonicalPosition(Position position) {
//     char board[boardSize];
//     GenericHashUnhash(position, board);

//     char pieceInSymmetry, pieceInCurrentCanonical;
//     int i, symmetryNum;
//     int turn = GenericHashGetTurn(position);

//     /* Figure out which symmetry transformation on the input board
//     leads to the smallest-ternary-number board in the input board's orbit
//     (where the transformations are just rotation/reflection. */
//     int bestSymmetryNum = 0;
//     for (symmetryNum = 1; symmetryNum < totalNumBoardSymmetries; symmetryNum++) {
//         for (i = boardSize - 1; i >= 0; i--) {
//             pieceInSymmetry = board[symmetries[symmetryNum][i]];
//             pieceInCurrentCanonical = board[symmetries[bestSymmetryNum][i]];
//             if (pieceInSymmetry != pieceInCurrentCanonical) {
//                 if (pieceInSymmetry < pieceInCurrentCanonical) {
//                     bestSymmetryNum = symmetryNum;
//                 }
//                 break;
//             }
//         }
//     }
//     char canonBoard[boardSize];
//     for (i = 0; i < boardSize; i++) { // Transform the rest of the board.
//         canonBoard[i] = board[symmetries[bestSymmetryNum][i]];
//     }

//     // Invert the piece colors of the board
//     for (i = 0; i < boardSize; i++) {
//         if (board[i] == W) {
//             board[i] = B;
//         } else if (board[i] == B) {
//             board[i] = W;
//         }
//     }

//     /* Figure out which symmetry transformation on the swapped input board
//     leads to the smallest-ternary-number board in the input board's orbit
//     (where the transformations are just rotation/reflection. */
//     bestSymmetryNum = 0;
//     for (symmetryNum = 1; symmetryNum < totalNumBoardSymmetries; symmetryNum++) {
//         for (i = boardSize - 1; i >= 0; i--) {
//             pieceInSymmetry = board[symmetries[symmetryNum][i]];
//             pieceInCurrentCanonical = board[symmetries[bestSymmetryNum][i]];
//             if (pieceInSymmetry != pieceInCurrentCanonical) {
//                 if (pieceInSymmetry < pieceInCurrentCanonical) {
//                     bestSymmetryNum = symmetryNum;
//                 }
//                 break;
//             }
//         }
//     }
//     char canonSwappedBoard[boardSize];
//     for (i = 0; i < boardSize; i++) { // Transform the rest of the board.
//         canonSwappedBoard[i] = board[symmetries[bestSymmetryNum][i]];
//     }

//     // Compare canonBoard and canonSwappedBoard
//     char pieceInRegular, pieceInSwapped;
//     for (i = boardSize - 1; i >= 0; i--) {
//         pieceInRegular = canonBoard[i];
//         pieceInSwapped = canonSwappedBoard[i];
//         if (pieceInRegular < pieceInSwapped) {
//             return GenericHashHash(canonBoard, turn);
//         } else if (pieceInSwapped < pieceInRegular) {
//             return GenericHashHash(canonSwappedBoard, turn == 1 ? 2 : 1);
//         }
//     }
//     return GenericHashHash(canonBoard, 1);
// }


// static PositionArray MkaooaGetCanonicalParentPositions(
//     Position position) {
//     /* The parent positions can be found by swapping the turn of
//     the position to get position P', getting the children of
//     P', canonicalizing them, then swapping the turn of each
//     of those canonical children. */

//     char board[boardSize];
//     GenericHashUnhash(position, board);
//     int t = GenericHashGetTurn(position);
//     int oppT = t == 1 ? 2 : 1;
//     Position turnSwappedPos = GenericHashHash(board, oppT);

//     PositionHashSet deduplication_set;
//     PositionHashSetInit(&deduplication_set, 0.5);

//     PositionArray canonicalParents;
//     PositionArrayInit(&canonicalParents);

//     Position child;
//     MoveArray moves = MkaooaGenerateMoves(turnSwappedPos);
//     for (int64_t i = 0; i < moves.size; i++) {
//         child = MkaooaDoMove(turnSwappedPos, moves.array[i]);
//         /* Note that at this point, it is current player's turn at `child`.
//         We check if it's primitive before we swap the turn because
//         primitive doesn't care about turn. */
//         if (MkaooaPrimitive(child) == kUndecided) {
//             GenericHashUnhash(child, board);
//             child = GenericHashHash(board, oppT);  // Now it's opponent's turn
//             child = MkaooaGetCanonicalPosition(child);
//             if (!PositionHashSetContains(&deduplication_set, child)) {
//                 PositionHashSetAdd(&deduplication_set, child);
//                 PositionArrayAppend(&canonicalParents, child);
//             }
//         }
//     }
//     PositionHashSetDestroy(&deduplication_set);
//     MoveArrayDestroy(&moves);
//     return canonicalParents;
// }


// TODO: Functiont to represent the board within GamesmanOne

static int MkaooaPositionToString(Position position, char *buffer) {
    char board[boardSize];
    GenericHashUnhash(position, board);
    char turn = GenericHashGetTurn(position) == 1 ? C : V;

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
                "MkaooaTierPositionToString: (BUG) not enough space "
                "was allocated "
                "to buffer. Please increase position_string_length_max.\n");
        return 1;
    }
    return 0;
}

static int MkaooaMoveToString(Move move, char *buffer) {
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
            "MkaooaMoveToString: (BUG) not enough space was allocated "
            "to buffer. Please increase move_string_length_max.\n");
        return 1;
    }
    return 0;
}

static bool MkaooaIsValidMoveString(ReadOnlyString move_string) {
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

static Move MkaooaStringToMove(ReadOnlyString move_string) {
    assert(MkaooaIsValidMoveString(move_string));

    int fromRow = move_string[0] - '1';
    int fromCol = move_string[1] - 'a';
    int from = fromRow * sideLength + fromCol;

    int toRow = move_string[2] - '1';
    int toCol = move_string[3] - 'a';
    int to = toRow * sideLength + toCol;

    return MOVE_ENCODE(from, to);
}

static void UnhashMove(Move move, int *from, int *to) {
    *from = move >> 5;
    *to = move & 0x1F;
}

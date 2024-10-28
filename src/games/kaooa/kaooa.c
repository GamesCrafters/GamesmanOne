/**
 * @file mkaooa.c
 * @author
 *  Xiang Zheng
 *  Sriya Kantipudi
 *  Maria Rufova
 *  Benji Xu
 *  Robert Shi
 * Supervised by Dan Garcia
 * @brief Kaooa implementation
 * @version 1.0.2
 * @date 2024-10-18
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

#include <assert.h>  // assert
#include <stdbool.h> // bool, true, false
#include <stddef.h>  // NULL
#include <stdint.h>  // int64_t
#include <stdio.h>   // fprintf, stderr
#include <stdlib.h>  // atoi

#include "core/generic_hash/generic_hash.h"
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/types/gamesman_types.h"

// Game, Solver, and Gameplay API Functions

#define boardSize 10

// MB TODO: Can we just use All Quenes Chess's unhash move and hash move?
#define MOVE_ENCODE(from, to) ((from << 5) | to)

// NOTE: Player 1 is always (C)row, Plaer 2 is always (V)ulture
// Player 1 Crow goes first!
#define C 'C'
#define V 'V'
#define BLANK '-'
#define CROWS_DROPPED_COUNT '0'

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
    .GetCanonicalPosition = &MkaooaGetCanonicalPosition, // MB TODO: Can change this to null?
    .GetNumberOfCanonicalChildPositions = NULL,
    .GetCanonicalChildPositions = NULL,
    .GetCanonicalParentPositions = &MkaooaGetCanonicalParentPositions, // MB TODO: Can change this to null?
};

// Gameplay API Setup

static const GameplayApiCommon kGamePlayApiCommon = {
    .GetInitialPosition = &MkaooaGetInitialPosition,
    .position_string_length_max = 120,

    .move_string_length_max = 3,
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

// Helper Functions

// this is good
static void UnhashMove(Move move, int *from, int *to);

static int MkaooaInit(void *aux)
{
    (void)aux; // Unused.

    GenericHashReinitialize();
    // MB TODO: Figure out what this array should be
    int pieces_init_array[13] = {BLANK, 10, 10, C, 0, 6, V, 0, 1, CROWS_DROPPED_COUNT, 0, 6, -1};
    bool success =
        GenericHashAddContext(0, boardSize, pieces_init_array, NULL, 0);
    if (!success)
    {
        fprintf(stderr,
                "Mkaooa: failed to initialize generic hash context. "
                "Aborting...\n");
        GenericHashReinitialize();
        return kRuntimeError;
    }
    return kNoError;
}

static int MkaooaFinalize(void) { return kNoError; }

static const GameVariant *MkaooaGetCurrentVariant(void)
{
    return NULL; // Later MB TODO
}

// start off with 6 crows
static int MkaooaSetVariantOption(int option, int selection)
{
    (void)option;
    (void)selection;
    return 0; // Later MB TODO
}

// TODO: Hash initial board configuration
static Position MkaooaGetInitialPosition(void)
{
    char *initial_str = "-----------";
    return GenericHashHash(initial_str, 1); // HINT: second parameter should be left as 1
                                            // WBWBW-----B---W-----BWBWB", 1); // HINT: second parameter should be left as 1
    // change this to our gameboard, so just put all dashes here
}

static int64_t MkaooaGetNumPositions(void)
{
    return GenericHashNumPositions();
}

// TODO
// Given a board position, generate all the moves available depending on whose turn it is.
// Position is a 64 bit integer. For us, we will use the last 11 digut of this number.
// The 11th digit would represent the # Crows dropped (0 - 6), and rest 10 digit would each correspond to a spot on the board (0 or 1 or 2)
int count_char_in_board(char board[], char c)
{
    int count = 0;
    for (int i = 0; i < boardSize; i++)
    {
        if (board[i] == c)
        {
            count++;
        }
    }
    return count;
}

int check_upper_bound(int n_1, int bound)
{
    return (n_1 > bound) ? -1 : 0;
}

int check_lower_bound(int n_1, int bound)
{
    return (n_1 < bound) ? 1 : 0;
}

// TODO
// Check 1027
static MoveArray MkaooaGenerateMoves(Position position)
{
    MoveArray moves;
    MoveArrayInit(&moves);

    char board[boardSize];
    GenericHashUnhash(position, board);

    char turn = GenericHashGetTurn(position) == 1 ? C : V;

    // NOTE: The following is an example of how possible moves were calculated for a piece in All Queens Chess.
    // You will not need to write as much because pieces in Kaooa generally have much less moves available to them.
    // You do not need to change the code above
    bool can_drop = (board[10] - '0') < 6;
    int move_count;
    int *possible_moves;
    for (int i = 0; i < boardSize; i++)
    {
        // C's turn
        if (turn == C && board[i] == C && !can_drop)
        // C's turn and can't drop
        {

            if (i < 5)
            {
                // out-circle:
                move_count = 2;
                possible_moves = (int *)malloc(move_count * sizeof(int));
                possible_moves[0] = i + 5 + check_lower_bound(i + 5, 5) * 5;
                possible_moves[1] = i + 4 + check_lower_bound(i + 4, 5) * 5;
            }
            else
            {
                // in-circle:
                move_count = 4;
                possible_moves = (int *)malloc(move_count * sizeof(int));
                possible_moves[0] = i - 5;
                possible_moves[1] = i - 1 + check_lower_bound(i - 1, 5) * 5;
                possible_moves[2] = i + 1 + check_upper_bound(i + 1, 9) * 5;
                possible_moves[3] = i - 4 + check_upper_bound(i - 4, 4) * 5;
            }
            for (int j = 0; j < move_count; i++)
            {
                if (board[possible_moves[j]] == BLANK)
                {
                    MoveArrayAppend(&moves, MOVE_ENCODE(j, possible_moves[j]));
                }
            }
        }
        else if (turn == C && board[i] == BLANK && can_drop)
        // C's turn and can drop
        {
            // Drop a crow
            MoveArrayAppend(&moves, MOVE_ENCODE(i, i));
        }
        // V's turn
        else if (turn == V && board[i] == V)
        {
            // TODO
            if (i < 5)
            {
                // out-circle:
                // direct move
                move_count = 2;
                possible_moves = (int *)malloc(move_count * sizeof(int));
                possible_moves[0] = i + 5 + check_lower_bound(i + 5, 5) * 5;
                possible_moves[1] = i + 4 + check_lower_bound(i + 4, 5) * 5;

                for (int k = 0; k < move_count; k++)
                {
                    if (board[possible_moves[k]] == BLANK)
                    {
                        MoveArrayAppend(&moves, MOVE_ENCODE(i, possible_moves[k]));
                    }
                }

                // jump move
                int *adjacent_positions = (int *)malloc(2 * sizeof(int));
                adjacent_positions[0] = i + 4 + check_lower_bound(i + 4, 5) * 5;
                adjacent_positions[1] = i + 5 + check_lower_bound(i + 5, 5) * 5;

                if (board[adjacent_positions[0]] == C)
                {
                    int jump_position = (adjacent_positions[0] - 1) + check_lower_bound(adjacent_positions[0] - 1, 5) * 5;
                    if (board[jump_position] == BLANK)
                    {
                        MoveArrayAppend(&moves, MOVE_ENCODE(i, jump_position));
                    }
                }
                if (board[adjacent_positions[1]] == C)
                {
                    int jump_position = (adjacent_positions[1] + 1) + check_upper_bound(adjacent_positions[1] + 1, 9) * 5;
                    if (board[jump_position] == BLANK)
                    {
                        MoveArrayAppend(&moves, MOVE_ENCODE(i, jump_position));
                    }
                }
            }
            else
            {
                // in-circle:
                // direct move
                move_count = 4;
                possible_moves = (int *)malloc(move_count * sizeof(int));
                possible_moves[0] = i - 5;
                possible_moves[1] = i - 1 + check_lower_bound(i - 1, 5) * 5;
                possible_moves[2] = i + 1 + check_upper_bound(i + 1, 9) * 5;
                possible_moves[3] = i - 4 + check_upper_bound(i - 4, 4) * 5;

                for (int k = 0; k < move_count; k++)
                {
                    if (board[possible_moves[k]] == BLANK)
                    {
                        MoveArrayAppend(&moves, MOVE_ENCODE(i, possible_moves[k]));
                    }
                }

                // jump move
                int *adjacent_positions = (int *)malloc(2 * sizeof(int));
                adjacent_positions[0] = i + 1 + check_upper_bound(i + 1, 9) * 5;
                adjacent_positions[1] = i - 1 + check_lower_bound(i - 1, 5) * 5;
                if (board[adjacent_positions[0]] == C)
                {
                    int jump_position = (adjacent_positions[0] - 6) + check_lower_bound(adjacent_positions[0] - 6, 0) * 5;
                    if (board[jump_position] == BLANK)
                    {
                        MoveArrayAppend(&moves, MOVE_ENCODE(i, jump_position));
                    }
                }
                if (board[adjacent_positions[1]] == C)
                {
                    int jump_position = (adjacent_positions[1] - 3) + check_upper_bound(adjacent_positions[1] + 1, 4) * 5;
                    if (board[jump_position] == BLANK)
                    {
                        MoveArrayAppend(&moves, MOVE_ENCODE(i, jump_position));
                    }
                }
            }
        }
    }
    free(possible_moves);

    return moves;
}

// TODO: Checks if a given board position is Primitive.
// A Primitive position is one that signals the end of the game.
// Hint: At what point do we know that the game is lost?
// The game ends when either the vulture captures 3 crows OR when the vulture is trapped
// For our game, we would only return kLose or kUndecided (reasons explained during meeting)
// check 1027
static Value MkaooaPrimitive(Position position)
{
    char board[boardSize];
    GenericHashUnhash(position, board);

    if ((board[10] - '0') - count_char_in_board(board, C) >= 3)
    {
        return kLose;
    }

    for (int i = 0; i < 10; i++)
    {
        if (board[i] == V)
        {
            if (i < 5)
            {
                int *possible_trap = (int *)malloc(4 * sizeof(int));
                possible_trap[0] = i + 4 + check_lower_bound(i + 4, 5) * 5;
                possible_trap[1] = i + 5;
                possible_trap[2] = i + 6 + check_upper_bound(i + 6, 9) * 5;
                possible_trap[3] = i + 3 + check_lower_bound(i + 3, 5) * 5;
                for (int j = 0; j < 4; j++)
                {
                    if (board[possible_trap[j]] == BLANK)
                    {
                        return kUndecided;
                    }
                }
                return kLose;
            }
            if (i >= 5)
            {
                int *impossible_trap = (int *)malloc(4 * sizeof(int));
                impossible_trap[0] = (i - 2) % 5;
                impossible_trap[1] = (i - 2) % 5 + 5;
                impossible_trap[2] = (i - 2) % 5 + 4 + check_lower_bound((i - 2) % 5 + 4, 5) * 5;
                impossible_trap[3] = i;
                int *possible_trap = (int *)malloc(6 * sizeof(int));
                int count = 0;
                for (int j = 0; j < 10; j++)
                {
                    if (!is_in_impossible_trap(impossible_trap, 4, j))
                    {
                        possible_trap[count++] = j;
                    }
                }
                for (int j = 0; j < 6; j++)
                {
                    if (board[possible_trap[j]] == BLANK)
                    {
                        return kUndecided;
                    }
                }
                return kLose;
            }
        }
    }

    return kUndecided;
}

// TODO: Takes in a Position and a Move, return a new Position (hashed) which is generated from performing the MOVE to POSITION
// Refer to psuedocode in slack!

int is_in_adjacent_positions(int *adjacent_positions, int size, int value)
{
    for (int i = 0; i < size; i++)
    {
        if (adjacent_positions[i] == value)
        {
            return 1;
        }
    }
    return 0;
}

// Check 1027
static Position MkaooaDoMove(Position position, Move move)
{
    char board[boardSize];
    GenericHashUnhash(position, board);

    int from, to;
    UnhashMove(move, &from, &to);
    int oppTurn = GenericHashGetTurn(position) == 1 ? C : V;
    // The code above can be left unchanged
    if (oppTurn == C)
    {
        if (from == to)
        {
            board[to] = C;
            board[10] = (board[10] - '0' + 1) + '0';
        }
        else
        {
            board[to] = C;
            board[from] = BLANK;
        }
    }
    else
    {
        // same to direct move & jump move
        board[to] = V;
        board[from] = BLANK;
        // check if is direct move
        if (from < 5)
        {
            int *adjacent_positions = (int *)malloc(2 * sizeof(int));
            adjacent_positions[0] = from + 4 + check_lower_bound(from + 4, 5) * 5;
            adjacent_positions[1] = from + 4 + check_lower_bound(from + 5, 5) * 5;
            if (!is_in_adjacent_positions(*adjacent_positions, 2, to))
            // jump move
            {
                int min = from < to ? from : to;
                int max = from < to ? to : from;
                int min_norm = max + check_upper_bound(max, 3) * 5;
                int diff = abs(to - from);
                if (diff == 6)
                {
                    int medium = min + 5 + check_lower_bound(min + 5, 5) * 5;
                    if (board[medium] == C)
                    {
                        board[medium] = BLANK;
                    }
                }
                else if (diff == 8 || diff == 3)
                {
                    int medium = min + 4 + check_lower_bound(min + 4, 5) * 5;
                    if (board[medium] == C)
                    {
                        board[medium] = BLANK;
                    }
                }
                if (max == 5 && min == 4)
                {
                    int medium = 9;
                    if (board[medium] == C)
                    {
                        board[medium] = BLANK;
                    }
                }
            }
        }
    }

    // int *adjacent_positions = (int *)malloc(2 * sizeof(int));
    //             adjacent_positions[0] = i + 1 + check_upper_bound(i + 1, 9) * 5;
    //             adjacent_positions[1] = i - 1 + check_lower_bound(i - 1, 5) * 5;
    //             if (board[adjacent_positions[0]] == C)
    //             {
    //                 int jump_position = (adjacent_positions[0] - 6) + check_lower_bound(adjacent_positions[0] - 6, 0) * 5;
    //                 if (board[jump_position] == BLANK)
    //                 {
    //                     MoveArrayAppend(&moves, MOVE_ENCODE(i, jump_position));
    //                 }
    //             }
    //             if (board[adjacent_positions[1]] == C)
    //             {
    //                 int jump_position = (adjacent_positions[1] - 3) + check_upper_bound(adjacent_positions[1] + 1, 4) * 5;
    //                 if (board[jump_position] == BLANK)
    //                 {
    //                     MoveArrayAppend(&moves, MOVE_ENCODE(i, jump_position));
    //                 }
    //             }

    int oppTurn = GenericHashGetTurn(position) == 1 ? C : V;
    return GenericHashHash(board, oppTurn);
}

static bool MkaooaIsLegalPosition(Position position)
{ // MB TODO: Do we need to implement this?
    // Don't need to implement.
    (void)position;
    return true;
}

// MB TODO: Not considering symmetries rn, but think about if we actually need this

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

// TODO: Takes in a POSITION, fills its string representation in the BUFFER.
// This is to display the board/position to the user when using GamesmanOne
static int MkaooaPositionToString(Position position, char *buffer)
{
    char board[boardSize];
    GenericHashUnhash(position, board);
    char turn = GenericHashGetTurn(position) == 1 ? C : V;

    static ConstantReadOnlyString kFormat =
        "\n"
        "1            [0]                           %c                 \\n"
        "            /  \\                         /  \\               \n"
        "2  [4]____[9]___[5]___[1]       %c______%c___%c_____%c       \n"
        "   \\    /       \\    /           \\    /       \\   /        \n"
        "     \\ /         \\ /             \\ /         \\ /         \n"
        "3     [8]__________[6]               %c___________%c          \n"
        "      / \\        / \\               / \\        / \\         \n"
        "     /   \\      /   \\             /   \\      /   \\        \n"
        "4   /       [7]       \\           /    /   %c       \\       \n"
        "   /    /         \\   \\         /    /         \\   \\      \n"
        "  /   /             \\  \\       /   /             \\  \\     \n"
        "5 [3]                 [2]        %c                  %c      \n"
        "           LEGEND                        TURN: %c"
        "                                                               ";

    int actual_length = snprintf(
        buffer, kGamePlayApiCommon.position_string_length_max + 1, kFormat,
        board[0], board[1], board[2], board[3], board[4], board[5], board[6],
        board[7], board[8], board[9], board[10], turn);

    if (actual_length >= kGamePlayApiCommon.position_string_length_max + 1)
    {
        fprintf(stderr,
                "MkaooaTierPositionToString: (BUG) not enough space "
                "was allocated "
                "to buffer. Please increase position_string_length_max.\n");
        return 1;
    }
    return 0;
}

// TODO: Takes in a MOVE, fills its string representation in the BUFFER.
// This is to display the move to the user when using GamesmanOne
// The string representation of a move can be a 2-character string seperated by a white space. Eg:
// 'X Y', where X is the source (0 - 9) and Y is the destination (0 - 9)
// When X = Y, the move signifies dropping a piece
static int MkaooaMoveToString(Move move, char *buffer)
{
    int from, to;
    UnhashMove(move, &from, &to);

    if (from == to)
    {
        int actual_length = snprintf(
            buffer, kGamePlayApiCommon.move_string_length_max + 1, "drop %d",
            from + 1);

        if (actual_length >= kGamePlayApiCommon.move_string_length_max + 1)
        {
            fprintf(
                stderr,
                "MkaooaMoveToString: (BUG) not enough space was allocated "
                "to buffer. Please increase move_string_length_max.\n");
            return 1;
        }

        return 0;
    }

    int actual_length = snprintf(
        buffer, kGamePlayApiCommon.move_string_length_max + 1, "%d-%d", from + 1, to + 1);
    if (actual_length >= kGamePlayApiCommon.move_string_length_max + 1)
    {
        fprintf(
            stderr,
            "MkaooaMoveToString: (BUG) not enough space was allocated "
            "to buffer. Please increase move_string_length_max.\n");
        return 1;
    }

    return 0;
}

// TODO
// Checks if string representing a move is valid
// This is NOT the same as checking if a move is a valid move. Here you are only supposed to check if a string is in the correct form
static bool MkaooaIsValidMoveString(ReadOnlyString move_string)
{
    if (move_string[0] - '0' < 0 || move_string[0] - '0' > 9)
    {
        return false;
    }
    else if (move_string[1] - '0' < 0 || move_string[1] - '0' > 9)
    {
        return false;
    }
    else
    {
        return true;
    }
}

// TODO: Converts the string move a user entered into a Move gamesmanone can understand internally.
static Move MkaooaStringToMove(ReadOnlyString move_string)
{
    assert(MkaooaIsValidMoveString(move_string));

    int from = move_string[0];
    int to = move_string[1];

    return MOVE_ENCODE(from, to);
}

// MB TODO: Can we just use All Queens Chess's unhash move and hash move?
// If not, consult Robert/Quixo
static void UnhashMove(Move move, int *from, int *to)
{
    *from = move >> 5;
    *to = move & 0x1F;
}

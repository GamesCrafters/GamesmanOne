/**
 * @file mkaooa.c
 * @author Xiang Zheng
 * @author Sriya Kantipudi
 * @author Maria Rufova
 * @author Benji Xu
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Kaooa implementation
 * @version 1.0.1
 * @date 2024-12-10
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

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // atoi
#include <string.h>   // strcpy, strlen

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

#define MAX_CROW_COUNT 6
#define MAX_VULTURE_COUNT 1

static int MkaooaInit(void *aux);
static int MkaooaFinalize(void);

// static const GameVariant *MkaooaGetCurrentVariant(void);
// static int MkaooaSetVariantOption(int option, int selection);

static int64_t MkaooaGetNumPositions(void);
static Position MkaooaGetInitialPosition(void);

static MoveArray MkaooaGenerateMoves(Position position);
static Value MkaooaPrimitive(Position position);
static Position MkaooaDoMove(Position position, Move move);
static bool MkaooaIsLegalPosition(Position position);
// static Position MkaooaGetCanonicalPosition(Position position);
// static PositionArray MkaooaGetCanonicalParentPositions(
//     Position position);

static int MkaooaPositionToString(Position position, char *buffer);
static int MkaooaMoveToString(Move move, char *buffer);
static bool MkaooaIsValidMoveString(ReadOnlyString move_string);
static Move MkaooaStringToMove(ReadOnlyString move_string);
bool is_in_impossible_trap(int impossible_trap[], int size, int value);

// Solver API Setup
static const RegularSolverApi kSolverApi = {
    .GetNumPositions = &MkaooaGetNumPositions,
    .GetInitialPosition = &MkaooaGetInitialPosition,
    .GenerateMoves = &MkaooaGenerateMoves,
    .Primitive = &MkaooaPrimitive,
    .DoMove = &MkaooaDoMove,
    .IsLegalPosition = &MkaooaIsLegalPosition,
    // .GetCanonicalPosition = &MkaooaGetCanonicalPosition,
    .GetNumberOfCanonicalChildPositions = NULL,
    .GetCanonicalChildPositions = NULL,
    // .GetCanonicalParentPositions = &MkaooaGetCanonicalParentPositions,
};

// Gameplay API Setup

static const GameplayApiCommon kGamePlayApiCommon = {
    .GetInitialPosition = &MkaooaGetInitialPosition,
    .position_string_length_max = 1028,

    .move_string_length_max = 128,
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

// Helper Functions

// Variants
// static ConstantReadOnlyString kMkaooaRuleChoices[] = {
//     "6 crows",
//     "7 crows",
// };

// static const GameVariantOption kMkaooaRules = {
//     .name = "rules",
//     .num_choices = sizeof(kMkaooaRuleChoices) /
//     sizeof(kMkaooaRuleChoices[0]), .choices = kMkaooaRuleChoices,
// };

// M: Zero terminator ??
#define NUM_OPTIONS 2  // 1 option and 1 zero-terminator
// static GameVariantOption kaooa_variant_options[NUM_OPTIONS];
// static int kaooa_variant_option_selections[NUM_OPTIONS] = {0, 0}; // 6 crows,
// 7 crows
#undef NUM_OPTIONS
// static GameVariant current_variant = {
//     .options = kaooa_variant_options,
//     .selections = kaooa_variant_option_selections,
// };

// this is good
static void UnhashMove(Move move, int *from, int *to);

static int MkaooaInit(void *aux) {
    (void)aux;  // Unused.

    GenericHashReinitialize();
    // MB TODO: Figure out what this array should be
    // int pieces_init_array[13] = {BLANK, 10, 10, C, 0, 6, V, 0, 1,
    // CROWS_DROPPED_COUNT, 0, 6, -1};
    static const int pieces_init_array[13] = {
        BLANK, 3, 10, C, 0, MAX_CROW_COUNT, V, 0, MAX_VULTURE_COUNT,
        -2,    0, 6,  -1};
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

// static const GameVariant *MkaooaGetCurrentVariant(void)
// {
//     return NULL; // Later MB TODO
// }

// start off with 6 crows
//

// TODO: Hash initial board configuration
static Position MkaooaGetInitialPosition(void) {
    char board[11] = "----------";
    board[10] = 0;  // using an unordered piece, should be int not char (look in
                    // generic_hash.h for more info)
    return GenericHashHash(board, 1);
}

static int64_t MkaooaGetNumPositions(void) { return GenericHashNumPositions(); }

// TODO
// Given a board position, generate all the moves available depending on whose
// turn it is. Position is a 64 bit integer. For us, we will use the last 11
// digut of this number. The 11th digit would represent the # Crows dropped (0 -
// 6), and rest 10 digit would each correspond to a spot on the board (0 or 1 or
// 2)
int count_char_in_board(char board[], char c) {
    int count = 0;
    for (int i = 0; i < boardSize; i++) {
        if (board[i] == c) count++;
    }
    return count;
}

// -1 if n1 > bound else 0
int check_upper_bound(int n_1, int bound) { return (n_1 > bound) ? -1 : 0; }

// 1 if n1 < bound else 0
int check_lower_bound(int n_1, int bound) { return (n_1 < bound) ? 1 : 0; }

// TODO
// Check 1027
static MoveArray MkaooaGenerateMoves(Position position) {
    MoveArray moves;
    MoveArrayInit(&moves);

    // printf("\nIn generate move");

    char board[boardSize + 1];
    GenericHashUnhash(position, board);
    char turn = GenericHashGetTurn(position) == 1 ? C : V;

    // NOTE: The following is an example of how possible moves were calculated
    // for a piece in All Queens Chess. You will not need to write as much
    // because pieces in Kaooa generally have much less moves available to them.
    // You do not need to change the code above
    bool can_drop = board[10] < 6;
    int move_count;
    int possible_moves[4];

    for (int i = 0; i < boardSize; i++) {
        // C's turn
        if (turn == C && board[i] == C && !can_drop)
        // C's turn and can't drop
        {
            if (i < 5) {
                // out-circle:
                move_count = 2;
                possible_moves[0] = i + 5 + check_lower_bound(i + 5, 5) * 5;
                possible_moves[1] = i + 4 + check_lower_bound(i + 4, 5) * 5;
            } else {
                // in-circle:
                move_count = 4;
                possible_moves[0] = i - 5;
                possible_moves[1] = i - 1 + check_lower_bound(i - 1, 5) * 5;
                possible_moves[2] = i + 1 + check_upper_bound(i + 1, 9) * 5;
                possible_moves[3] = i - 4 + check_upper_bound(i - 4, 4) * 5;
            }
            for (int j = 0; j < move_count; j++) {
                if (board[possible_moves[j]] == BLANK) {
                    MoveArrayAppend(&moves, MOVE_ENCODE(i, possible_moves[j]));
                }
            }
        } else if (turn == C && board[i] == BLANK && can_drop) {
            // C's turn and can drop
            // Drop a crow
            MoveArrayAppend(&moves, MOVE_ENCODE(i, i));
        } else if (turn == V && board[i] == BLANK &&
                   count_char_in_board(board, V) == 0) {
            // V's turn
            // 1029
            // printf("\nDEBUG: V's turn and no vultures on the board\n");
            // Drop a vulture
            MoveArrayAppend(&moves, MOVE_ENCODE(i, i));
        } else if (turn == V && board[i] == V) {
            // TODO
            if (i < 5) {
                // out-circle:
                // direct move
                move_count = 2;
                possible_moves[0] = i + 5 + check_lower_bound(i + 5, 5) * 5;
                possible_moves[1] = i + 4 + check_lower_bound(i + 4, 5) * 5;

                for (int k = 0; k < move_count; k++) {
                    if (board[possible_moves[k]] == BLANK) {
                        MoveArrayAppend(&moves,
                                        MOVE_ENCODE(i, possible_moves[k]));
                    }
                }

                // jump move
                int adjacent_positions[2];
                adjacent_positions[0] = i + 4 + check_lower_bound(i + 4, 5) * 5;
                adjacent_positions[1] = i + 5 + check_lower_bound(i + 5, 5) * 5;

                if (board[adjacent_positions[0]] == C) {
                    int jump_position =
                        (adjacent_positions[0] - 1) +
                        check_lower_bound(adjacent_positions[0] - 1, 5) * 5;
                    if (board[jump_position] == BLANK) {
                        MoveArrayAppend(&moves, MOVE_ENCODE(i, jump_position));
                    }
                }
                if (board[adjacent_positions[1]] == C) {
                    int jump_position =
                        (adjacent_positions[1] + 1) +
                        check_upper_bound(adjacent_positions[1] + 1, 9) * 5;
                    if (board[jump_position] == BLANK) {
                        MoveArrayAppend(&moves, MOVE_ENCODE(i, jump_position));
                    }
                }
            } else {
                // in-circle:
                // direct move
                move_count = 4;
                possible_moves[0] = i - 5;
                possible_moves[1] = i - 1 + check_lower_bound(i - 1, 5) * 5;
                possible_moves[2] = i + 1 + check_upper_bound(i + 1, 9) * 5;
                possible_moves[3] = i - 4 + check_upper_bound(i - 4, 4) * 5;

                for (int k = 0; k < move_count; k++) {
                    if (board[possible_moves[k]] == BLANK) {
                        MoveArrayAppend(&moves,
                                        MOVE_ENCODE(i, possible_moves[k]));
                    }
                }

                // jump move
                int adjacent_positions[2];
                adjacent_positions[0] = i - 1 + check_lower_bound(i - 1, 5) * 5;
                adjacent_positions[1] = i + 1 + check_upper_bound(i + 1, 9) * 5;
                if (board[adjacent_positions[0]] == C) {
                    int jump_position =
                        (adjacent_positions[0] - 5) +
                        check_lower_bound(adjacent_positions[0] - 5, 0) * 5;
                    if (board[jump_position] == BLANK) {
                        MoveArrayAppend(&moves, MOVE_ENCODE(i, jump_position));
                    }
                }
                if (board[adjacent_positions[1]] == C) {
                    int jump_position =
                        (adjacent_positions[1] - 4) +
                        check_upper_bound(adjacent_positions[1] - 4, 4) * 5;
                    if (board[jump_position] == BLANK) {
                        MoveArrayAppend(&moves, MOVE_ENCODE(i, jump_position));
                    }
                }
            }
        }
    }

    // printf("DEBUG: End of generate moves");

    return moves;
}

// TODO: Checks if a given board position is Primitive.
// A Primitive position is one that signals the end of the game.
// Hint: At what point do we know that the game is lost?
// The game ends when either the vulture captures 3 crows OR when the vulture is
// trapped For our game, we would only return kLose or kUndecided (reasons
// explained during meeting) check 1027

bool is_in_impossible_trap(int impossible_trap[], int size, int value) {
    for (int i = 0; i < size; i++) {
        if (impossible_trap[i] == value) {
            return true;
        }
    }
    return false;
}

static Value MkaooaPrimitive(Position position) {
    char board[boardSize + 1];
    GenericHashUnhash(position, board);

    // char another_board[boardSize + 1];
    // GenericHashUnhash(position, another_board);

    if (board[10] - count_char_in_board(board, C) >= 3) {
        return kLose;
    }

    for (int i = 0; i < 10; i++) {
        if (board[i] == V) {
            if (i < 5) {
                int possible_trap[4];
                possible_trap[0] = i + 4 + check_lower_bound(i + 4, 5) * 5;
                possible_trap[1] = i + 5;
                possible_trap[2] = i + 6 + check_upper_bound(i + 6, 9) * 5;
                possible_trap[3] = i + 3 + check_lower_bound(i + 3, 5) * 5;
                for (int j = 0; j < 4; j++) {
                    if (board[possible_trap[j]] == BLANK) {
                        return kUndecided;
                    }
                }
                return kLose;
            }
            if (i >= 5) {
                int impossible_trap[4];
                impossible_trap[0] = (i - 2) % 5;
                impossible_trap[1] = (i - 2) % 5 + 5;
                impossible_trap[2] =
                    (i - 2) % 5 + 4 + check_lower_bound((i - 2) % 5 + 4, 5) * 5;
                impossible_trap[3] = i;
                int possible_trap[6];
                int count = 0;
                for (int j = 0; j < 10; j++) {
                    if (!is_in_impossible_trap(impossible_trap, 4, j)) {
                        possible_trap[count++] = j;
                    }
                }
                for (int j = 0; j < 6; j++) {
                    if (board[possible_trap[j]] == BLANK) {
                        return kUndecided;
                    }
                }
                return kLose;
            }
        }
    }

    return kUndecided;
}

// TODO: Takes in a Position and a Move, return a new Position (hashed) which is
// generated from performing the MOVE to POSITION Refer to psuedocode in slack!

int is_in_adjacent_positions(int *adjacent_positions, int size, int value) {
    for (int i = 0; i < size; i++) {
        if (adjacent_positions[i] == value) {
            return 1;
        }
    }
    return 0;
}

// Check 1027
static Position MkaooaDoMove(Position position, Move move) {
    char board[boardSize + 1];
    GenericHashUnhash(position, board);

    // char another_board[boardSize + 1];
    // GenericHashUnhash(position, another_board);

    int from, to;
    UnhashMove(move, &from, &to);
    int oppTurn = GenericHashGetTurn(position) == 1 ? C : V;
    // The code above can be left unchanged
    if (oppTurn == C) {
        if (from == to)  // dropping crow
        {
            // condition to check if TO is blank not needed --> checked in
            // GenMoves
            board[to] = C;
            board[10] = (char)(board[10] + 1);
            return GenericHashHash(board, oppTurn == C ? 2 : 1);
        } else {  // moving crow
            board[to] = C;
            board[from] = BLANK;
            return GenericHashHash(board, oppTurn == C ? 2 : 1);
        }
    } else {
        // V's turn
        if (from == to) {
            board[to] = V;

            return GenericHashHash(board, oppTurn == C ? 2 : 1);
        }
        // same to direct move & jump move
        board[to] = V;
        board[from] = BLANK;
        // check if is direct move
        int min_pos = from < to ? from : to;
        if (min_pos > 4) {
            return GenericHashHash(board, oppTurn == C ? 2 : 1);
        } else {
            int adjacent_positions[2];
            adjacent_positions[0] =
                from + 4 + check_lower_bound(from + 4, 5) * 5;
            adjacent_positions[1] =
                from + 4 + check_lower_bound(from + 5, 5) * 5;
            if (is_in_adjacent_positions(adjacent_positions, 2, to)) {
                return GenericHashHash(board, oppTurn == C ? 2 : 1);
            }
            if (!is_in_adjacent_positions(adjacent_positions, 2, to)) {
                // jump move
                // the move here already obeyed the rule
                int min = from < to ? from : to;
                int diff = abs(to - from);

                if (diff % 5 == 1) {  // left jump
                    int medium = min + 5 + check_lower_bound(min + 5, 5) * 5;
                    if (board[medium] == C) {
                        board[medium] = BLANK;
                    } else {
                        printf(
                            "Error: Thought there was a crow here, but there "
                            "wasn't\n");
                    }
                } else if (diff % 5 == 3) {  // right jump
                    int medium = min + 4 + check_lower_bound(min + 4, 5) * 5;
                    if (board[medium] == C) {
                        board[medium] = BLANK;
                    } else {
                        printf(
                            "Error: Thought there was a crow here, but there "
                            "wasn't\n");
                    }
                }
            }
        }

        return GenericHashHash(board, oppTurn == C ? 2 : 1);
    }
}

static bool MkaooaIsLegalPosition(Position position) {
    char board[boardSize + 1];
    GenericHashUnhash(position, board);
    int crow_count = 0;
    for (int i = 0; i < 10; ++i) {
        crow_count += board[i] == C;
    }

    // Positions with more crows on board than ever placed are invalid.
    return crow_count <= board[10];
}

// MB TODO: Not considering symmetries rn, but think about if we actually need
// this

// static Position MkaooaGetCanonicalPosition(Position position) {
//     char board[boardSize];
//     GenericHashUnhash(position, board);

//     return GenericHashHash(canonBoard, 1);
// }

// static PositionArray MkaooaGetCanonicalParentPositions(
//     Position position) {
//     return canonicalParents;
// }

// TODO: Takes in a POSITION, fills its string representation in the BUFFER.
// This is to display the board/position to the user when using GamesmanOne
static int MkaooaPositionToString(Position position, char *buffer) {
    // printf("\nIn PositionToString");
    char board[boardSize + 1];
    GenericHashUnhash(position, board);
    char turn = GenericHashGetTurn(position) == 1 ? C : V;

    static ConstantReadOnlyString kFormat =
        "\n"
        "            [0]                          %c  \n"
        "           /  \\                         /  \\  \n"
        "   [4]___[9]___[5]___[1]         %c____%c_____%c_____%c       \n"
        "   \\    /       \\    /           \\   /       \\   /        \n"
        "     \\ /         \\  /             \\ /         \\ /         \n"
        "      [8]        [6]               %c           %c          \n"
        "      / \\        / \\               / \\        / \\         \n"
        "     /   \\      /   \\             /   \\      /   \\        \n"
        "    /      [7]       \\           /    /   %c  \\    \\       \n"
        "   /    /         \\   \\         /    /         \\   \\      \n"
        "  /   /             \\  \\       /   /             \\  \\     \n"
        "  [3]                 [2]        %c                  %c      \n"
        "           LEGEND                        TURN: %c           \n"
        "EATEN CROWS: %d\n";

    int eaten_crows = board[10] - count_char_in_board(board, C);
    int actual_length = snprintf(
        buffer, kGamePlayApiCommon.position_string_length_max + 1, kFormat,
        board[0], board[4], board[9], board[5], board[1], board[8], board[6],
        board[7], board[3], board[2], turn, eaten_crows);

    if (actual_length >= kGamePlayApiCommon.position_string_length_max + 1) {
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
// The string representation of a move can be a 2-character string seperated by
// a white space. Eg: 'X Y', where X is the source (0 - 9) and Y is the
// destination (0 - 9) When X = Y, the move signifies dropping a piece
static int MkaooaMoveToString(Move move, char *buffer) {
    // printf("\nIn MoveToString");
    int from, to;
    UnhashMove(move, &from, &to);

    if (from == to) {
        int actual_length =
            snprintf(buffer, kGamePlayApiCommon.move_string_length_max + 1,
                     "%d %d", from, from);

        if (actual_length >= kGamePlayApiCommon.move_string_length_max + 1) {
            fprintf(stderr,
                    "MkaooaMoveToString: (BUG) not enough space was allocated "
                    "to buffer. Please increase move_string_length_max.\n");
            return 1;
        }

        return 0;
    }

    int actual_length =
        snprintf(buffer, kGamePlayApiCommon.move_string_length_max + 1, "%d %d",
                 from, to);
    if (actual_length >= kGamePlayApiCommon.move_string_length_max + 1) {
        fprintf(stderr,
                "MkaooaMoveToString: (BUG) not enough space was allocated "
                "to buffer. Please increase move_string_length_max.\n");
        return 1;
    }

    return 0;
}

// TODO
// Checks if string representing a move is valid
// This is NOT the same as checking if a move is a valid move. Here you are only
// supposed to check if a string is in the correct form
static bool MkaooaIsValidMoveString(ReadOnlyString move_string) {
    // printf("\nIn IsValidMoveString");
    if (move_string[0] - '0' < 0 || move_string[0] - '0' > 9) {
        return false;
    } else if (move_string[2] - '0' < 0 || move_string[2] - '0' > 9) {
        return false;
    } else {
        return true;
    }
}

// TODO: Converts the string move a user entered into a Move gamesmanone can
// understand internally.
static Move MkaooaStringToMove(ReadOnlyString move_string) {
    // printf("\nIn StringToMove");
    assert(MkaooaIsValidMoveString(move_string));

    int from = move_string[0] - '0';
    int to = move_string[2] - '0';

    return MOVE_ENCODE(from, to);
}

// MB TODO: Can we just use All Queens Chess's unhash move and hash move?
// If not, consult Robert/Quixo
static void UnhashMove(Move move, int *from, int *to) {
    *from = (int)(move >> 5);
    *to = (int)(move & 0x1F);
}

/////////////////////////UWAPI Set-up//////////////////////////

static bool KaooaIsLegalFormalPosition(ReadOnlyString formal_position);
static Position KaooaFormalPositionToPosition(ReadOnlyString formal_position);
static CString KaooaPositionToFormalPosition(Position position);
static CString KaooaMoveToFormalMove(Position position, Move move);

static CString KaooaPositionToAutoGuiPosition(Position position);
static CString KaooaMoveToAutoGuiMove(Position position, Move move);

static const UwapiRegular kKaooaUwapiRegular = {
    .GenerateMoves = MkaooaGenerateMoves,
    .DoMove = MkaooaDoMove,
    .Primitive = MkaooaPrimitive,

    .IsLegalFormalPosition = KaooaIsLegalFormalPosition,
    .FormalPositionToPosition = KaooaFormalPositionToPosition,
    .PositionToFormalPosition = KaooaPositionToFormalPosition,
    .MoveToFormalMove = KaooaMoveToFormalMove,

    .PositionToAutoGuiPosition = KaooaPositionToAutoGuiPosition,
    .MoveToAutoGuiMove = KaooaMoveToAutoGuiMove,
    .GetInitialPosition = MkaooaGetInitialPosition,

    .GetRandomLegalPosition = NULL,  // Not available for this game.
};

static const Uwapi kKaooaUwapi = {.regular = &kKaooaUwapiRegular};

int kKaooaFormalPositionStrlen = 12;

// C -> 0, V -> 1 , all others mapped to -1. Initialized by InitPieceToIndex.
// static int8_t kPieceToIndex[1];

// static void InitPieceToIndex(void) {
//     kPieceToIndex[(int)'C'] = 0;
//     kPieceToIndex[(int)'V'] = 1;
// }

// static bool KaooaIsValidConfig(const int *config);

static bool KaooaIsLegalFormalPosition(ReadOnlyString formal_position) {
    // String length must match regular format.
    (void)formal_position;
    return true;
}

static Position KaooaFormalPositionToPosition(ReadOnlyString formal_position) {
    //
    char board[11];
    memcpy(board, formal_position + 2, boardSize);

    board[10] = (char)(formal_position[12] - '0');

    return GenericHashHash(board, formal_position[0] - '0');
}

static CString KaooaPositionToFormalPosition(Position position) {
    return KaooaPositionToAutoGuiPosition(position);
}
// AutoGui position format:
// """
// [turn]_[board (10x)]
// """
static CString KaooaPositionToAutoGuiPosition(Position position) {
    char board[11];
    GenericHashUnhash(position, board);
    CString ret;
    char autogui_position[] = "1_----------";
    autogui_position[0] = (char)('0' + GenericHashGetTurn(position));
    for (int i = 0; i <= 9; ++i) {
        if (board[i] == V) {
            autogui_position[i + 2] = 'V';
        } else if (board[i] == C) {
            autogui_position[i + 2] = 'C';
        }
    }
    CStringInitCopyCharArray(&ret, autogui_position);
    char n[2];
    n[0] = (char)(board[10] + '0');
    n[1] = '\0';
    CStringAppend(&ret, n);
    return ret;
}
// static void UnpackMove(Move move, int *src, int *dest) {
//     *dest = move % boardSize;
//     *src = move / boardSize;
// }

static CString KaooaMoveToFormalMove(Position position, Move move) {
    (void)position;

    int from, to;
    UnhashMove(move, &from, &to);
    CString ret;
    char formal_move[7];

    if (from == to) {  // dropping
        formal_move[0] = (char)('0' + from);
        formal_move[1] = '\0';
    } else {  // moving
        formal_move[0] = (char)('0' + from);
        formal_move[1] = ' ';
        formal_move[2] = '-';
        formal_move[3] = '>';
        formal_move[4] = ' ';
        formal_move[5] = (char)('0' + to);
        formal_move[6] = '\0';
    }

    CStringInitCopyCharArray(&ret, formal_move);
    return ret;
}

static CString KaooaMoveToAutoGuiMove(Position position, Move move) {
    (void)position;
    int from, to;
    UnhashMove(move, &from, &to);
    char autogui_move[] = "A_-_3";
    CString ret;
    if (from == to) {
        autogui_move[4] = (char)('0' + from);
    } else {
        // sprintf(autogui_move, "M_%d_%d", from, to);
        autogui_move[0] = 'M';
        autogui_move[2] = (char)('0' + from);
        autogui_move[4] = (char)('0' + to);
    }
    CStringInitCopyCharArray(&ret, autogui_move);
    return ret;
}

const Game kMkaooa = {
    .name = "mkaooa",
    .formal_name = "Kaooa",
    .solver = &kRegularSolver,
    .solver_api = (const void *)&kSolverApi,
    .gameplay_api = (const GameplayApi *)&kGameplayApi,
    .uwapi = &kKaooaUwapi,

    .Init = &MkaooaInit,
    .Finalize = &MkaooaFinalize,

    .GetCurrentVariant = NULL,
    .SetVariantOption = NULL,
};

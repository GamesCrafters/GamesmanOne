#include "core/interactive/games/presolve/postsolve/play/play.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // exit, EXIT_FAILURE
#include <string.h>   // strncmp

#include "core/constants.h"
#include "core/interactive/games/presolve/match.h"
#include "core/misc.h"
#include "core/solvers/solver_manager.h"
#include "core/types/gamesman_types.h"

static bool solved;
static int lose_children_remoteness_min;
static int tie_children_remoteness_min;
static int win_children_remoteness_max;
static Int64HashMap move_values, move_remotenesses;

static void PrintPrediction(void) {
    int turn = InteractiveMatchGetTurn();
    bool is_computer = InteractiveMatchPlayerIsComputer(turn);
    ReadOnlyString controller = is_computer ? "Computer" : "Human";
    ReadOnlyString prediction = is_computer ? "will" : "should";

    TierPosition current = InteractiveMatchGetCurrentPosition();
    Value value = SolverManagerGetValue(current);
    char value_string[32];
    switch (value) {
        case kUndecided:
            printf("Current position has undecided value.\n");
            return;

        case kWin:
            sprintf(value_string, "win");
            break;

        case kTie:
            sprintf(value_string, "tie");
            break;

        case kDraw:
            printf("Player 1 and Player 2 are in a draw.\n");
            return;

        case kLose:
            sprintf(value_string, "lose");
            break;

        default:
            printf(
                "An error occurred on probing the value for the current "
                "position.\n");
            return;
    }

    int remoteness = SolverManagerGetRemoteness(current);
    printf("Player %d (%s) %s %s in %d.", turn + 1, controller, prediction,
           value_string, remoteness);
}

static void PrintCurrentPosition(const Game *game) {
    int position_string_size =
        game->gameplay_api->common->position_string_length_max + 1;
    char *position_string =
        (char *)SafeMalloc(position_string_size * sizeof(char));

    TierPosition current = InteractiveMatchGetCurrentPosition();
    int error = InteractiveMatchPositionToString(current, position_string);
    if (error < 0) {
        fprintf(stderr,
                "PlayTierGame: %s's PositionToString function returned error "
                "code %d. Aborting...\n",
                game->formal_name, error);
        exit(EXIT_FAILURE);
    }
    printf("%s\t", position_string);
    free(position_string);
    if (solved) PrintPrediction();
    printf("\n");
}

/**
 * @brief This function relies on the extreme remoteness global
 * variables being set and assumes a current-legal-moves-available
 * context. Given the move's child positon's value and remoteness,
 * output a number that serves to help rank this move among all
 * currently available legal moves. The higher the number, the worse
 * the move is. This is different from deltaRemoteness in that
 * deltaRemoteness only provides a ranking among all moves of the
 * same value, whereas this provides a ranking among all moves.
 */
int GetMoveRank(Value childValue, int remoteness) {
    if (childValue == kLose) {
        return remoteness - lose_children_remoteness_min;
    } else if (childValue == kTie) {
        return remoteness - tie_children_remoteness_min + kRemotenessMax;
    } else if (childValue == kDraw) {
        return kRemotenessMax * 2;
    } else if (childValue == kWin) {
        return win_children_remoteness_max - remoteness + kRemotenessMax * 3;
    } else {
        return 0;
    }
}

/**
 * @brief Move Comparison helper function for PrintSortedMoveValues.
 * Assuming m1 and m2 have the same value, return positive
 * if m1 is better remoteness-wise, negative if m2 is better
 * remoteness-wise, and 0 otherwise.
 *
 * @note If m1 and m2 have different values, this function
 * will say that they're equal.
 */
int MoveCompare(const void *move1, const void *move2) {
    Move m1 = *((Move *)move1);
    Move m2 = *((Move *)move2);
    Int64HashMapIterator it = Int64HashMapGet(&move_values, m1);
    Value value1 = Int64HashMapIteratorValue(&it);
    it = Int64HashMapGet(&move_values, m2);
    Value value2 = Int64HashMapIteratorValue(&it);
    it = Int64HashMapGet(&move_remotenesses, m1);
    int remoteness1 = Int64HashMapIteratorValue(&it);
    it = Int64HashMapGet(&move_remotenesses, m2);
    int remoteness2 = Int64HashMapIteratorValue(&it);

    return GetMoveRank(value1, remoteness1) - GetMoveRank(value2, remoteness2);
}

/**
 * @brief Helper function for PrintSortedMoveValues.
 */
static void PrintMovesOfValue(const Game *game, MoveArray *moves,
                              TierPosition current, char *move_string,
                              Value childValue) {
    for (int64_t i = 0; i < moves->size; i++) {
        TierPosition child = InteractiveMatchDoMove(current, moves->array[i]);
        if (childValue == SolverManagerGetValue(child)) {
            game->gameplay_api->common->MoveToString(moves->array[i],
                                                     move_string);
            if (childValue == kDraw) {
                printf("\t\t\t%-16s\tDraw\n", move_string);
            } else {
                int remoteness = SolverManagerGetRemoteness(child);
                printf("\t\t\t%-16s\t%d\n", move_string, remoteness);
            }
        }
    }
}

/**
 * @brief Set the extreme child remoteness global variables by finding
 * the extreme remotenesses of all child positions resulting from the
 * moves to the passed in move array.
 */
void SetExtremeChildRemotenesses(TierPosition current, MoveArray *moves) {
    // Initialize extreme remotenesses
    lose_children_remoteness_min = kRemotenessMax;
    tie_children_remoteness_min = kRemotenessMax;
    win_children_remoteness_max = 0;

    for (int64_t i = 0; i < moves->size; i++) {
        TierPosition child = InteractiveMatchDoMove(current, moves->array[i]);
        int remoteness = SolverManagerGetRemoteness(child);
        switch (SolverManagerGetValue(child)) {
            case kWin:
                if (remoteness > win_children_remoteness_max) {
                    win_children_remoteness_max = remoteness;
                }
                break;
            case kTie:
                if (remoteness < tie_children_remoteness_min) {
                    tie_children_remoteness_min = remoteness;
                }
                break;
            case kLose:
                if (remoteness < lose_children_remoteness_min) {
                    lose_children_remoteness_min = remoteness;
                }
                break;
            default:
                break;
        }
    }
}

static void MoveValueCacheCleanup(void) {
    if (move_values.size) Int64HashMapDestroy(&move_values);
    memset(&move_values, 0, sizeof(move_values));
    if (move_remotenesses.size) Int64HashMapDestroy(&move_remotenesses);
    memset(&move_remotenesses, 0, sizeof(move_remotenesses));
}

static bool LoadMoveValues(const MoveArray *moves) {
    MoveValueCacheCleanup();
    Int64HashMapInit(&move_values, 0.5);
    Int64HashMapInit(&move_remotenesses, 0.5);
    for (int64_t i = 0; i < moves->size; ++i) {
        TierPosition current = InteractiveMatchGetCurrentPosition();
        TierPosition child = InteractiveMatchDoMove(current, moves->array[i]);
        Value value = SolverManagerGetValue(child);
        if (!Int64HashMapSet(&move_values, moves->array[i], value)) {
            fprintf(stderr,
                    "LoadMoveValues: failed to create new map entry for move "
                    "value\n");
            return false;
        }
        int remoteness = SolverManagerGetRemoteness(child);
        if (!Int64HashMapSet(&move_remotenesses, moves->array[i], remoteness)) {
            fprintf(stderr,
                    "LoadMoveValues: failed to create new map entry for move "
                    "remoteness\n");
            return false;
        }
    }

    return true;
}

/**
 * @brief Print moves in order sorted from best to worst. Value/remoteness
 * sorted from best to worst is as follows: low-remoteness win,
 * high-remoteness win, low-remoteness tie, high-remoteness tie, draw,
 * high-remoteness lose, low-remoteness lose. Credit to @Jiong for original
 * GamemsanClassic SortedMoveValues printout design.
 */
static bool PrintSortedMoveValues(const Game *game) {
    int move_string_size =
        game->gameplay_api->common->move_string_length_max + 1;
    char *move_string = (char *)SafeMalloc(move_string_size * sizeof(char));

    TierPosition current = InteractiveMatchGetCurrentPosition();
    MoveArray moves = InteractiveMatchGenerateMoves();
    if (!LoadMoveValues(&moves)) return false;
    SetExtremeChildRemotenesses(current, &moves);
    MoveArraySortExplicit(&moves, MoveCompare);

    printf("\n\t==========================================================\n");
    printf("\n\t\tHere are the values of all possible moves:\n\n");
    printf("\t\t\tMove            \tRemoteness\n");
    printf("\t\tWinning:\n");
    PrintMovesOfValue(game, &moves, current, move_string, kLose);
    printf("\t\tTying:\n");
    PrintMovesOfValue(game, &moves, current, move_string, kTie);
    PrintMovesOfValue(game, &moves, current, move_string, kDraw);
    printf("\t\tLosing:\n");
    PrintMovesOfValue(game, &moves, current, move_string, kWin);
    printf("\n\t==========================================================\n");
    printf("\n");

    free(move_string);
    MoveArrayDestroy(&moves);
    return true;
}

static bool IsBestChild(Value parent_value, int parent_remoteness,
                        Value child_value, int child_remoteness) {
    switch (parent_value) {
        case kLose:
            assert(child_value == kWin);
            return (child_remoteness == parent_remoteness - 1);

        case kWin:
            return (child_value == kLose &&
                    child_remoteness == parent_remoteness - 1);

        case kTie:
            assert(child_value != kLose);
            return (child_value == kTie &&
                    child_remoteness == parent_remoteness - 1);

        case kDraw:
            assert(child_value != kLose);
            return (child_value == kDraw);

        default:
            NotReached("IsBestChild: unknown parent value.\n");
    }
    return false;
}

// This function should not be called if the current game has not been solved.
static void MakeComputerMove(void) {
    TierPosition current = InteractiveMatchGetCurrentPosition();
    MoveArray moves = InteractiveMatchGenerateMoves();
    Value current_value = SolverManagerGetValue(current);
    int current_remoteness = SolverManagerGetRemoteness(current);

    for (int64_t i = 0; i < moves.size; ++i) {
        TierPosition child = InteractiveMatchDoMove(current, moves.array[i]);
        Value value = SolverManagerGetValue(child);
        int remoteness = SolverManagerGetRemoteness(child);
        if (IsBestChild(current_value, current_remoteness, value, remoteness)) {
            InteractiveMatchCommitMove(moves.array[i]);
            break;
        }
    }
    MoveArrayDestroy(&moves);
}

static bool PromptForAndProcessUserMove(const Game *game) {
    int move_string_size =
        game->gameplay_api->common->move_string_length_max + 2;
    char *move_string = (char *)SafeMalloc(move_string_size * sizeof(char));
    MoveArray moves = InteractiveMatchGenerateMoves();

    // Print all valid move strings.
    printf("Player %d's move [(u)ndo", InteractiveMatchGetTurn() + 1);
    if (solved) printf("/(v)alues");
    for (int64_t i = 0; i < moves.size; ++i) {
        game->gameplay_api->common->MoveToString(moves.array[i], move_string);
        printf("/[%s]", move_string);
    }
    printf("]: ");

    // Prompt for input.
    if (fgets(move_string, move_string_size, stdin) == NULL) {
        fprintf(stderr, "PlayTierGame: unexpected fgets error. Aborting...\n");
        exit(EXIT_FAILURE);
    }
    move_string[strcspn(move_string, "\r\n")] = '\0';

    if (strncmp(move_string, "v", 1) == 0) {  // Print values for all moves.
        if (solved) {
            PrintSortedMoveValues(game);
        } else {
            printf("Game is not solved, so move values cannot be shown.\n");
        }
        return true;
    }
    if (strncmp(move_string, "q", 1) == 0) GamesmanExit();  // Exit GAMESMAN.
    if (strncmp(move_string, "b", 1) == 0) return true;     // Exit game.
    if (strncmp(move_string, "u", 1) == 0) {                // Undo.
        return InteractiveMatchUndo();
    }
    if (!game->gameplay_api->common->IsValidMoveString(move_string)) {
        printf("Sorry, I don't know that option. Try another.\n");
        free(move_string);
        return false;
    }
    Move user_move = game->gameplay_api->common->StringToMove(move_string);
    if (!MoveArrayContains(&moves, user_move)) {
        printf("Sorry, I don't know that option. Try another.\n");
        free(move_string);
        return false;
    }
    MoveArrayDestroy(&moves);
    free(move_string);
    InteractiveMatchCommitMove(user_move);
    return true;
}

static void PrintGameResult(ReadOnlyString game_formal_name) {
    Value value = InteractiveMatchPrimitive();
    int turn = InteractiveMatchGetTurn();
    switch (value) {
        case kUndecided:
            fprintf(stderr,
                    "PlayGame: (BUG) game ended at a non-primitive position. "
                    "Check the implementation of gameplay. Aborting...\n");
            exit(EXIT_FAILURE);
            break;
        case kLose:
            printf("Player %d wins!\n", (!turn) + 1);
            break;
        case kWin:
            printf("Player %d wins!\n", turn + 1);
            break;
        case kTie:
            printf(
                "The match ends in a tie. Excellent strategies, Player 1 and "
                "Player 2!");
            break;
        default:
            // including kDraw, which should never be returned as a primitive
            // value.
            fprintf(
                stderr,
                "PlayGame: (BUG) game ended at a position of unknown value %d. "
                "Check the implementation of %s. Aborting...\n",
                (int)value, game_formal_name);
            exit(EXIT_FAILURE);
            break;
    }
}

int InteractivePlay(ReadOnlyString key) {
    (void)key;  // Unused.
    memset(&move_values, 0, sizeof(move_values));
    memset(&move_remotenesses, 0, sizeof(move_remotenesses));

    if (!InteractiveMatchRestart()) {
        fprintf(stderr,
                "InteractivePlay: (BUG) attempting to launch game when the "
                "game is uninitialized. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    const Game *game = InteractiveMatchGetCurrentGame();
    solved = InteractiveMatchSolved();
    PrintCurrentPosition(game);
    Value primitive_value = InteractiveMatchPrimitive();
    bool game_over = (primitive_value != kUndecided);

    while (!game_over) {
        int turn = InteractiveMatchGetTurn();
        if (InteractiveMatchPlayerIsComputer(turn)) {
            // Generate computer move
            MakeComputerMove();
        } else if (!PromptForAndProcessUserMove(game)) {
            // If user entered an unknown command, restart the loop.
            continue;
        }
        // Else, move has been successfully processed. Print the new position.
        PrintCurrentPosition(game);
        primitive_value = InteractiveMatchPrimitive();
        game_over = (primitive_value != kUndecided);
    }
    PrintGameResult(game->formal_name);

    MoveValueCacheCleanup();
    return 0;
}

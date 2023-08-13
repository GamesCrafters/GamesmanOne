#include "core/interactive/games/presolve/postsolve/play/play.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // exit, EXIT_FAILURE
#include <string.h>   // strncmp

#include "core/db_manager.h"
#include "core/gamesman_types.h"  // Game
#include "core/interactive/games/presolve/match.h"
#include "core/misc.h"  // SafeMalloc, GameVariantToIndex

static void PrintCurrentPosition(const Game *game) {
    int position_string_size = game->gameplay_api->position_string_length_max;
    char *position_string =
        (char *)SafeMalloc(position_string_size * sizeof(char));

    TierPosition current = InteractiveMatchGetCurrentPosition();
    int result = InteractiveMatchPositionToString(current, position_string);
    if (result < 0) {
        fprintf(stderr,
                "PlayTierGame: %s's PositionToString function returned error "
                "code %d. Aborting...\n",
                game->formal_name, result);
        exit(EXIT_FAILURE);
    }
    printf("%s\n", position_string);
    free(position_string);
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

static void MakeComputerMove(const Game *game) {
    TierPosition current = InteractiveMatchGetCurrentPosition();
    MoveArray moves = InteractiveMatchGenerateMoves();
    Value current_value = DbManagerGetValue(current);
    int current_remoteness = DbManagerGetRemoteness(current);

    for (int64_t i = 0; i < moves.size; ++i) {
        TierPosition child = InteractiveMatchDoMove(current, moves.array[i]);
        Value value = DbManagerGetValue(child);
        int remoteness = DbManagerGetRemoteness(child);
        if (IsBestChild(current_value, current_remoteness, value, remoteness)) {
            InteractiveMatchCommitMove(moves.array[i]);
            break;
        }
    }
    MoveArrayDestroy(&moves);
}

static bool PromptForAndProcessUserMove(const Game *game) {
    int move_string_size = game->gameplay_api->move_string_length_max;
    char *move_string = (char *)SafeMalloc(move_string_size * sizeof(char));
    MoveArray moves = InteractiveMatchGenerateMoves();

    // Print all valid move strings.
    printf("Player %d's move [(u)ndo", InteractiveMatchGetTurn() + 1);
    for (int64_t i = 0; i < moves.size; ++i) {
        game->gameplay_api->MoveToString(moves.array[i], move_string);
        printf("/%s", move_string);
    }
    printf("]: ");

    // Prompt for input.
    if (fgets(move_string, move_string_size, stdin) == NULL) {
        fprintf(stderr, "PlayTierGame: unexpcted fgets error. Aborting...\n");
        exit(EXIT_FAILURE);
    }
    if (strncmp(move_string, "u", 1) == 0) {
        return InteractiveMatchUndo();
    }
    if (!game->gameplay_api->IsValidMoveString(move_string)) {
        printf("Sorry, I don't know that option. Try another.\n");
        free(move_string);
        return false;
    }
    Move user_move = game->gameplay_api->StringToMove(move_string);
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

static void PrintGameResult(const Game *game, bool is_tier_game) {
    TierPosition current_position = InteractiveMatchGetCurrentPosition();
    Value value = GetPrimitiveValue(game, is_tier_game);
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
                (int)value, game->formal_name);
            exit(EXIT_FAILURE);
            break;
    }
}

void InteractivePlay(const char *key) {
    (void)key;  // Unused.
    if (!InteractiveMatchRestart()) {
        fprintf(stderr,
                "InteractivePlay: (BUG) attempting to launch game when the "
                "game is uninitialized. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    const Game *game = InteractiveMatchGetCurrentGame();
    PrintCurrentPosition(game);
    Value primitive_value = GetPrimitiveValue(game);
    bool game_over = (primitive_value != kUndecided);

    while (!game_over) {
        int turn = InteractiveMatchGetTurn();
        if (InteractiveMatchPlayerIsComputer(turn)) {
            // Generate computer move
            MakeComputerMove(game);
        } else if (!PromptForAndProcessUserMove(game)) {
            // If user entered an unknown command, restart the loop.
            continue;
        }
        // Else, move has been successfully processed. Print the new position.
        PrintCurrentPosition(game);
        primitive_value = GetPrimitiveValue(game);
        game_over = (primitive_value != kUndecided);
    }
}

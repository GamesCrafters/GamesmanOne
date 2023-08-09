#include "core/interactive/games/presolve/postsolve/play/play.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // exit, EXIT_FAILURE

#include "core/gamesman_types.h"  // Game
#include "core/interactive/games/presolve/match.h"
#include "core/misc.h"  // SafeMalloc

static bool BasicApiImplemented(const GameplayApi *api) {
    // "Move to string" related functions.
    if (api->move_string_length_max <= 0) return false;
    if (api->MoveToString == NULL) return false;

    // "String to Move" related functions.
    if (api->IsValidMoveString == NULL) return false;
    if (api->StringToMove == NULL) return false;

    // Common functions to both APIs.
    if (api->position_string_length_max <= 0) return false;

    return true;
}

static bool TierApiImplemented(const GameplayApi *api) {
    if (!BasicApiImplemented(api)) return false;
    if (api->TierPositionToString == NULL) return false;
    if (api->TierGenerateMoves == NULL) return false;
    if (api->TierDoMove == NULL) return false;
    if (api->TierPrimitive == NULL) return false;

    return true;
}

static bool RegularApiImplemented(const GameplayApi *api) {
    if (!BasicApiImplemented(api)) return false;
    if (api->PositionToString == NULL) return false;
    if (api->GenerateMoves == NULL) return false;
    if (api->DoMove == NULL) return false;
    if (api->Primitive == NULL) return false;

    return true;
}

static void PrintCurrentPosition(const Game *game, bool is_tier_game) {
    int position_string_size = game->gameplay_api->position_string_length_max;
    char *position_string =
        (char *)SafeMalloc(position_string_size * sizeof(char));

    TierPosition current_position = InteractiveMatchGetCurrentPosition();
    int result;
    if (is_tier_game) {
        result = game->gameplay_api->TierPositionToString(
            current_position.tier, current_position.position, position_string);
    } else {
        result = game->gameplay_api->PositionToString(current_position.position,
                                                      position_string);
    }
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

static Value GetPrimitiveValue(const Game *game, bool is_tier_game) {
    TierPosition current_position = InteractiveMatchGetCurrentPosition();
    if (is_tier_game) {
        return game->gameplay_api->TierPrimitive(current_position.tier,
                                                 current_position.position);
    }
    return game->gameplay_api->Primitive(current_position.position);
}

static bool PromptForAndProcessUserMove(const Game *game, bool is_tier_game) {
    int move_string_size = game->gameplay_api->move_string_length_max;
    char *move_string = (char *)SafeMalloc(move_string_size * sizeof(char));
    TierPosition current_position = InteractiveMatchGetCurrentPosition();
    MoveArray moves;
    if (is_tier_game) {
        moves = game->gameplay_api->TierGenerateMoves(
            current_position.tier, current_position.position);
    } else {
        moves = game->gameplay_api->GenerateMoves(current_position.position);
    }

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
    InteractiveMatchDoMove(user_move);
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

static void PlayGame(const Game *game, bool is_tier_game) {
    InteractiveMatchRestart(game);
    PrintCurrentPosition(game, is_tier_game);
    Value primitive_value = GetPrimitiveValue(game, is_tier_game);
    bool game_over = (primitive_value != kUndecided);

    while (!game_over) {
        int turn = InteractiveMatchGetTurn();
        if (InteractiveMatchPlayerIsComputer(turn)) {
            // Generate computer move
        } else if (!PromptForAndProcessUserMove(game, is_tier_game)) {
            // If user entered an unknown command, restart the loop.
            continue;
        }
        // Else, move has been successfully processed. Print the new position.
        PrintCurrentPosition(game, is_tier_game);
        primitive_value = GetPrimitiveValue(game, is_tier_game);
        game_over = (primitive_value != kUndecided);
    }
}

void InteractivePlay(const char *key) {
    (void)key;  // Unused.
    const Game *current_game = InteractiveMatchGetCurrentGame();
    assert(current_game != NULL);
    if (current_game->gameplay_api == NULL) {
        fprintf(stderr,
                "InteractivePlay: %s does not have its Gameplay API "
                "initialized. Please check the game module.\n",
                current_game->formal_name);
        return;
    }
    if (RegularApiImplemented(current_game->gameplay_api)) {
        PlayGame(current_game, false);
    } else if (TierApiImplemented(current_game->gameplay_api)) {
        PlayGame(current_game, true);
    } else {
        fprintf(stderr,
                "InteractivePlay: %s's Gameplay API is incomplete. "
                "Please check the game module.\n",
                current_game->formal_name);
    }
}

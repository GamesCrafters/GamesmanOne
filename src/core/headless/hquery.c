#include "core/headless/hquery.h"

#include <assert.h>  // assert
#include <json-c/json_object.h>
#include <stdbool.h>  // bool, true, false
#include <stdint.h>   // int64_t
#include <stdio.h>    // printf

#include "core/constants.h"
#include "core/game_manager.h"
#include "core/headless/hjson.h"
#include "core/headless/hutils.h"
#include "core/solvers/solver_manager.h"
#include "core/types/gamesman_types.h"

static int InitAndCheckGame(ReadOnlyString game_name, int variant_id,
                            ReadOnlyString data_path, bool *is_tier_game);
static bool ImplementsRegularUwapi(const Game *game);
static bool ImplementsTierUwapi(const Game *game);

static int QueryRegular(const Game *game, ReadOnlyString formal_position);
static int QueryTier(const Game *game, ReadOnlyString formal_position);

static int GetStartRegular(const Game *game);
static int GetStartTier(const Game *game);

static int GetRandomRegular(const Game *game);
static int GetRandomTier(const Game *game);

static int JsonPrintPositionResponse(const Game *game, Position position);
static json_object *JsonCreateBasicPositionObject(const Game *game,
                                                  Position position);
static json_object *JsonCreateChildPositionObject(const Game *game,
                                                  Position parent, Move move);
static json_object *JsonCreateParentPositionObject(
    const Game *game, Position position, json_object *moves_array_obj);

static int JsonPrintTierPositionResponse(const Game *game,
                                         TierPosition tier_position);

static void JsonPrintStartResponse(ReadOnlyString start,
                                   ReadOnlyString autogui_start);
static void JsonPrintRandomResponse(ReadOnlyString random,
                                    ReadOnlyString autogui_random);
static void JsonPrintSinglePosition(ReadOnlyString position,
                                    ReadOnlyString autogui_position);

static void JsonPrintErrorResponse(ReadOnlyString message);

// -----------------------------------------------------------------------------

int HeadlessQuery(ReadOnlyString game_name, int variant_id,
                  ReadOnlyString data_path, ReadOnlyString formal_position) {
    bool is_tier_game;
    int error =
        InitAndCheckGame(game_name, variant_id, data_path, &is_tier_game);
    if (error != 0) return error;

    const Game *game = GameManagerGetCurrentGame();
    if (is_tier_game) return QueryTier(game, formal_position);
    return QueryRegular(game, formal_position);
}

int HeadlessGetStart(ReadOnlyString game_name, int variant_id) {
    bool is_tier_game;
    int error = InitAndCheckGame(game_name, variant_id, NULL, &is_tier_game);
    if (error != 0) return error;

    const Game *game = GameManagerGetCurrentGame();
    if (is_tier_game) return GetStartTier(game);
    return GetStartRegular(game);
}

int HeadlessGetRandom(ReadOnlyString game_name, int variant_id) {
    bool is_tier_game;
    int error = InitAndCheckGame(game_name, variant_id, NULL, &is_tier_game);
    if (error != 0) return error;

    const Game *game = GameManagerGetCurrentGame();
    if (is_tier_game) return GetRandomTier(game);
    return GetRandomRegular(game);
}

// -----------------------------------------------------------------------------

static int InitAndCheckGame(ReadOnlyString game_name, int variant_id,
                            ReadOnlyString data_path, bool *is_tier_game) {
    int error = HeadlessInitSolver(game_name, variant_id, data_path);
    if (error != 0) {
        JsonPrintErrorResponse("game initialization failed");
        return error;
    }

    const Game *game = GameManagerGetCurrentGame();
    assert(game != NULL);
    bool implements_regular = ImplementsRegularUwapi(game);
    bool implements_tier = ImplementsTierUwapi(game);
    if (!implements_regular && !implements_tier) {
        JsonPrintErrorResponse(
            "missing UWAPI function definition, check the game implementation");
        return -2;
    }

    *is_tier_game = implements_tier;
    return 0;
}

static bool ImplementsRegularUwapi(const Game *game) {
    if (game->uwapi == NULL) return false;
    if (game->uwapi->regular == NULL) return false;
    if (game->uwapi->regular->GenerateMoves == NULL) return false;
    if (game->uwapi->regular->DoMove == NULL) return false;
    if (game->uwapi->regular->FormalPositionToPosition == NULL) return false;
    if (game->uwapi->regular->PositionToFormalPosition == NULL) return false;
    if (game->uwapi->regular->PositionToAutoGuiPosition == NULL) return false;
    // MoveToFormalMove is optional.
    if (game->uwapi->regular->MoveToAutoGuiMove == NULL) return false;
    if (game->uwapi->regular->GetInitialPosition == NULL) return false;
    // GetRandomLegalPosition is optional.

    return true;
}

static bool ImplementsTierUwapi(const Game *game) {
    if (game->uwapi == NULL) return false;
    if (game->uwapi->tier == NULL) return false;
    if (game->uwapi->tier->GenerateMoves == NULL) return false;
    if (game->uwapi->tier->DoMove == NULL) return false;
    if (game->uwapi->tier->FormalPositionToTierPosition == NULL) return false;
    if (game->uwapi->tier->TierPositionToFormalPosition == NULL) return false;
    if (game->uwapi->tier->TierPositionToAutoGuiPosition == NULL) return false;
    // MoveToFormalMove is optional.
    if (game->uwapi->tier->MoveToAutoGuiMove == NULL) return false;
    if (game->uwapi->tier->GetInitialTierPosition == NULL) return false;
    // GetRandomLegalTierPosition is optional.

    return true;
}

static int QueryRegular(const Game *game, ReadOnlyString formal_position) {
    Position position =
        game->uwapi->regular->FormalPositionToPosition(formal_position);
    if (position < 0) {
        JsonPrintErrorResponse("illegal position");
        return 2;
    }
    return JsonPrintPositionResponse(game, position);
}

static int QueryTier(const Game *game, ReadOnlyString formal_position) {
    TierPosition tier_position =
        game->uwapi->tier->FormalPositionToTierPosition(formal_position);
    if (tier_position.tier < 0 || tier_position.position < 0) {
        JsonPrintErrorResponse("illegal position");
        return 2;
    }
    return JsonPrintTierPositionResponse(game, tier_position);
}

static int GetStartRegular(const Game *game) {
    Position start = game->uwapi->regular->GetInitialPosition();
    if (start < 0) {
        JsonPrintErrorResponse(
            "illegal initial position, please check the game implementation");
        return -1;
    }

    int ret = 0;
    CString formal_start =
        game->uwapi->regular->PositionToFormalPosition(start);
    CString autogui_start =
        game->uwapi->regular->PositionToAutoGuiPosition(start);
    if (formal_start.str == NULL || autogui_start.str == NULL) {
        JsonPrintErrorResponse("out of memory");
        ret = 1;
    } else {
        JsonPrintStartResponse(formal_start.str, autogui_start.str);
    }
    CStringDestroy(&formal_start);
    CStringDestroy(&autogui_start);

    return ret;
}

static int GetStartTier(const Game *game) {
    TierPosition start = game->uwapi->tier->GetInitialTierPosition();
    if (start.tier < 0 || start.position < 0) {
        JsonPrintErrorResponse(
            "illegal initial position, please check the game implementation");
        return -1;
    }

    int ret = 0;
    CString formal_start =
        game->uwapi->tier->TierPositionToFormalPosition(start);
    CString autogui_start =
        game->uwapi->tier->TierPositionToAutoGuiPosition(start);
    if (formal_start.str == NULL || autogui_start.str == NULL) {
        JsonPrintErrorResponse("out of memory");
        ret = 1;
    } else {
        JsonPrintStartResponse(formal_start.str, autogui_start.str);
    }
    CStringDestroy(&formal_start);
    CStringDestroy(&autogui_start);

    return ret;
}

static int GetRandomRegular(const Game *game) {
    if (game->uwapi->regular->GetRandomLegalPosition == NULL) {
        JsonPrintErrorResponse("position randomization not supported");
        return -2;
    }
    Position random = game->uwapi->regular->GetRandomLegalPosition();
    if (random < 0) {
        JsonPrintErrorResponse(
            "illegal initial position, please check the game implementation");
        return -1;
    }

    int ret = 0;
    CString formal_random =
        game->uwapi->regular->PositionToFormalPosition(random);
    CString autogui_random =
        game->uwapi->regular->PositionToAutoGuiPosition(random);
    if (formal_random.str == NULL || autogui_random.str == NULL) {
        JsonPrintErrorResponse("out of memory");
        ret = 1;
    } else {
        JsonPrintRandomResponse(formal_random.str, autogui_random.str);
    }
    CStringDestroy(&formal_random);
    CStringDestroy(&autogui_random);

    return ret;
}

static int GetRandomTier(const Game *game) {
    if (game->uwapi->tier->GetRandomLegalTierPosition == NULL) {
        JsonPrintErrorResponse("position randomization not supported");
        return -2;
    }
    TierPosition random = game->uwapi->tier->GetRandomLegalTierPosition();
    if (random.tier < 0 || random.position < 0) {
        JsonPrintErrorResponse(
            "illegal initial position, please check the game implementation");
        return -1;
    }

    int ret = 0;
    CString formal_random =
        game->uwapi->tier->TierPositionToFormalPosition(random);
    CString autogui_random =
        game->uwapi->tier->TierPositionToAutoGuiPosition(random);
    if (formal_random.str == NULL || autogui_random.str == NULL) {
        JsonPrintErrorResponse("out of memory");
        ret = 1;
    } else {
        JsonPrintRandomResponse(formal_random.str, autogui_random.str);
    }
    CStringDestroy(&formal_random);
    CStringDestroy(&autogui_random);

    return ret;
}

static int JsonPrintPositionResponse(const Game *game, Position position) {
    int ret = 0;
    MoveArray moves = game->uwapi->regular->GenerateMoves(position);
    json_object *moves_array_obj = NULL, *child_obj = NULL, *parent_obj = NULL;
    if (moves.array == NULL) {
        JsonPrintErrorResponse("out of memory");
        ret = 1;
        goto _bailout;
    }

    moves_array_obj = json_object_new_array_ext(moves.size);
    if (moves_array_obj == NULL) {
        JsonPrintErrorResponse("out of memory");
        ret = 1;
        goto _bailout;
    }

    for (int64_t i = 0; i < moves.size; ++i) {
        Move move = moves.array[i];
        child_obj = JsonCreateChildPositionObject(game, position, move);
        if (child_obj == NULL) {
            JsonPrintErrorResponse("out of memory");
            ret = 1;
            goto _bailout;
        }
        json_object_array_add(moves_array_obj, child_obj);
        child_obj = NULL;
    }

    parent_obj =
        JsonCreateParentPositionObject(game, position, moves_array_obj);
    if (parent_obj == NULL) {
        JsonPrintErrorResponse("out of memory");
        ret = 1;
        goto _bailout;
    }
    moves_array_obj = NULL;
    printf("%s\n", json_object_to_json_string(parent_obj));

_bailout:
    MoveArrayDestroy(&moves);
    json_object_put(moves_array_obj);
    json_object_put(child_obj);
    json_object_put(parent_obj);
    return ret;
}

static json_object *JsonCreateBasicPositionObject(const Game *game,
                                                  Position position) {
    json_object *ret = json_object_new_object();
    if (ret == NULL) goto _bailout;

    CString formal_position =
        game->uwapi->regular->PositionToFormalPosition(position);
    CString autogui_position =
        game->uwapi->regular->PositionToAutoGuiPosition(position);
    if (formal_position.str == NULL || autogui_position.str == NULL) {
        json_object_put(ret);
        ret = NULL;
        goto _bailout;
    }
    int error = HeadlessJsonAddPosition(ret, formal_position.str);
    error |= HeadlessJsonAddAutoGuiPosition(ret, autogui_position.str);

    TierPosition tier_position = {.tier = kDefaultTier, .position = position};
    Value value = SolverManagerGetValue(tier_position);
    int remoteness = SolverManagerGetRemoteness(tier_position);
    error |= HeadlessJsonAddValue(ret, value);
    error |= HeadlessJsonAddRemoteness(ret, remoteness);
    if (error) {
        json_object_put(ret);
        ret = NULL;
    }

_bailout:
    CStringDestroy(&formal_position);
    CStringDestroy(&autogui_position);
    return ret;
}

static json_object *JsonCreateChildPositionObject(const Game *game,
                                                  Position parent, Move move) {
    Position child = game->uwapi->regular->DoMove(parent, move);
    json_object *ret = JsonCreateBasicPositionObject(game, child);
    if (ret == NULL) return NULL;

    CString formal_move = game->uwapi->regular->MoveToFormalMove(parent, move);
    CString autogui_move =
        game->uwapi->regular->MoveToAutoGuiMove(parent, move);
    if (formal_move.str == NULL || autogui_move.str == NULL) {
        json_object_put(ret);
        return NULL;
    }

    int error = HeadlessJsonAddMove(ret, formal_move.str);
    error |= HeadlessJsonAddAutoGuiMove(ret, autogui_move.str);
    CStringDestroy(&formal_move);
    CStringDestroy(&autogui_move);
    if (error) {
        json_object_put(ret);
        return NULL;
    }

    return ret;
}

static json_object *JsonCreateParentPositionObject(
    const Game *game, Position position, json_object *moves_array_obj) {
    json_object *ret = JsonCreateBasicPositionObject(game, position);
    if (ret == NULL) return NULL;

    int error = HeadlessJsonAddMovesArray(ret, moves_array_obj);
    if (error) {
        json_object_put(ret);
        return NULL;
    }

    return ret;
}

// TODO: error checking
static int JsonPrintTierPositionResponse(const Game *game,
                                         TierPosition tier_position) {
    // TODO
    (void)game;
    (void)tier_position;
}

static void JsonPrintStartResponse(ReadOnlyString formal_start,
                                   ReadOnlyString autogui_start) {
    JsonPrintSinglePosition(formal_start, autogui_start);
}

static void JsonPrintRandomResponse(ReadOnlyString formal_random,
                                    ReadOnlyString autogui_random) {
    JsonPrintSinglePosition(formal_random, autogui_random);
}

static void JsonPrintSinglePosition(ReadOnlyString position,
                                    ReadOnlyString autogui_position) {
    json_object *response = json_object_new_object();
    HeadlessJsonAddPosition(response, position);
    HeadlessJsonAddAutoGuiPosition(response, autogui_position);
    printf("%s\n", json_object_to_json_string(response));
    json_object_put(response);
}

static void JsonPrintErrorResponse(ReadOnlyString message) {
    json_object *response = json_object_new_object();
    HeadlessJsonAddError(response, message);
    printf("%s\n", json_object_to_json_string(response));
    json_object_put(response);
}

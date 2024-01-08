#include "core/headless/hquery.h"

#include <assert.h>   // assert
#include <json-c/json_object.h>
#include <stdbool.h>  // bool, true, false
#include <stdio.h>    // printf, fprintf, stderr

#include "core/constants.h"
#include "core/db/db_manager.h"
#include "core/game_manager.h"
#include "core/headless/hutils.h"
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

// -----------------------------------------------------------------------------

int HeadlessQuery(ReadOnlyString game_name, int variant_id,
                  ReadOnlyString data_path, ReadOnlyString formal_position) {
    bool is_tier_game;
    int error =
        InitAndCheckGame(game_name, variant_id, data_path, &is_tier_game);
    if (error != 0) {
        // TODO: print out error json
        return error;
    }

    const Game *game = GameManagerGetCurrentGame();
    if (is_tier_game) return QueryTier(game, formal_position);
    return QueryRegular(game, formal_position);
}

int HeadlessGetStart(ReadOnlyString game_name, int variant_id) {
    bool is_tier_game;
    int error = InitAndCheckGame(game_name, variant_id, NULL, &is_tier_game);
    if (error != 0) {
        // TODO: print out error json
        return error;
    }

    const Game *game = GameManagerGetCurrentGame();
    if (is_tier_game) return GetStartTier(game);
    return GetStartRegular(game);
}

int HeadlessGetRandom(ReadOnlyString game_name, int variant_id) {
    bool is_tier_game;
    int error = InitAndCheckGame(game_name, variant_id, NULL, &is_tier_game);
    if (error != 0) {
        // TODO: print out error json
        return error;
    }

    const Game *game = GameManagerGetCurrentGame();
    if (is_tier_game) return GetRandomTier(game);
    return GetRandomRegular(game);
}

// -----------------------------------------------------------------------------

static int InitAndCheckGame(ReadOnlyString game_name, int variant_id,
                            ReadOnlyString data_path, bool *is_tier_game) {
    int error = HeadlessInitSolver(game_name, variant_id, data_path);
    if (error != 0) return error;

    const Game *game = GameManagerGetCurrentGame();
    assert(game != NULL);
    bool implements_regular = ImplementsRegularUwapi(game);
    bool implements_tier = ImplementsTierUwapi(game);
    if (!implements_regular && !implements_tier) return -2;

    *is_tier_game = implements_tier;
    return 0;
}

static bool ImplementsRegularUwapi(const Game *game) {
    if (game->uwapi == NULL) return false;
    if (game->uwapi->regular == NULL) return false;
    if (game->uwapi->regular->FormalPositionToPosition == NULL) return false;
    if (game->uwapi->regular->PositionToFormalPosition == NULL) return false;
    if (game->uwapi->regular->PositionToUwapiPosition == NULL) return false;
    if (game->uwapi->regular->MoveToUwapiMove == NULL) return false;

    return true;
}

static bool ImplementsTierUwapi(const Game *game) {
    if (game->uwapi == NULL) return false;
    if (game->uwapi->tier == NULL) return false;
    if (game->uwapi->tier->FormalPositionToTierPosition == NULL) return false;
    if (game->uwapi->tier->TierPositionToFormalPosition == NULL) return false;
    if (game->uwapi->tier->TierPositionToUwapiPosition == NULL) return false;
    if (game->uwapi->tier->TierMoveToUwapiMove == NULL) return false;

    return true;
}

static int QueryRegular(const Game *game, ReadOnlyString formal_position) {
    Position position =
        game->uwapi->regular->FormalPositionToPosition(formal_position);
    if (position < 0) {
        fprintf(stderr,
                "QueryRegular: failed to convert formal position [%s] into "
                "hashed Position.\n",
                formal_position);
        return 2;
    }

    DbProbe probe;
    DbManagerProbeInit(&probe);
    TierPosition tier_position = {.tier = kDefaultTier, .position = position};

    // TODO: parse this into the UWAPI json format
    Value value = DbManagerProbeValue(&probe, tier_position);
    printf("value: %s\n", kValueStrings[(int)value]);
    int remoteness = DbManagerProbeRemoteness(&probe, tier_position);
    printf("remoteness: %d\n", remoteness);
    DbManagerProbeDestroy(&probe);
    return 0;
}

static int QueryTier(const Game *game, ReadOnlyString formal_position) {
    TierPosition tier_position =
        game->uwapi->tier->FormalPositionToTierPosition(formal_position);
    if (tier_position.tier < 0 || tier_position.position < 0) {
        fprintf(stderr,
                "QueryTier: failed to convert formal position [%s] into "
                "hashed TierPosition.\n",
                formal_position);
        return 2;
    }

    DbProbe probe;
    DbManagerProbeInit(&probe);

    // TODO: parse this into the UWAPI json format
    Value value = DbManagerProbeValue(&probe, tier_position);
    printf("value: %s\n", kValueStrings[(int)value]);
    int remoteness = DbManagerProbeRemoteness(&probe, tier_position);
    printf("remoteness: %d\n", remoteness);
    DbManagerProbeDestroy(&probe);
    return 0;
}

static int GetStartRegular(const Game *game) {
    Position start = game->uwapi->regular->GetInitialPosition();
    if (start < 0) {
        // TODO: print out error json
        return -1;
    }

    // TODO: parse this into the UWAPI json format
    CString uwapi_start = game->uwapi->regular->PositionToUwapiPosition(start);
    // TODO: json error
    if (uwapi_start.length < 0) return 1;
    printf("%s\n", uwapi_start.str);
    CStringDestroy(&uwapi_start);
    return 0;
}

static int GetStartTier(const Game *game) {
    TierPosition start = game->uwapi->tier->GetInitialTierPosition();
    if (start.tier < 0 || start.position < 0) {
        // TODO: print out error json
        return -1;
    }

    // TODO: parse this into the UWAPI json format
    CString uwapi_start = game->uwapi->tier->TierPositionToUwapiPosition(start);
    // TODO: json error
    if (uwapi_start.length < 0) return 1;
    printf("%s\n", uwapi_start.str);
    CStringDestroy(&uwapi_start);
    return 0;
}

static int GetRandomRegular(const Game *game) {
    Position random = game->uwapi->regular->GetRandomLegalPosition();
    if (random < 0) {
        // TODO: print out error json
        return -1;
    }

    // TODO: parse this into the UWAPI json format
    CString uwapi_random =
        game->uwapi->regular->PositionToUwapiPosition(random);
    // TODO: json error
    if (uwapi_random.length < 0) return 1;
    printf("%s\n", uwapi_random.str);
    CStringDestroy(&uwapi_random);
    return 0;
}

static int GetRandomTier(const Game *game) {
    TierPosition random = game->uwapi->tier->GetRandomLegalTierPosition();
    if (random.tier < 0 || random.position < 0) {
        // TODO: print out error json
        return -1;
    }

    // TODO: parse this into the UWAPI json format
    CString uwapi_random =
        game->uwapi->tier->TierPositionToUwapiPosition(random);
    // TODO: json error
    if (uwapi_random.length < 0) return 1;
    printf("%s\n", uwapi_random.str);
    CStringDestroy(&uwapi_random);
    return 0;
}

#include "core/headless/hquery.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stdio.h>    // printf, fprintf, stderr

#include "core/constants.h"
#include "core/db/db_manager.h"
#include "core/gamesman_types.h"
#include "core/headless/hutils.h"
#include "games/game_manager.h"

static bool ImplementsBasicUwapi(const Game *game);
static bool ImplementsRegularUwapi(const Game *game);
static bool ImplementsTierUwapi(const Game *game);

static int QueryRegular(const Game *game, ReadOnlyString formal_position);
static int QueryTier(const Game *game, ReadOnlyString formal_position);

// -----------------------------------------------------------------------------

int HeadlessQuery(ReadOnlyString game_name, int variant_id,
                  ReadOnlyString data_path, ReadOnlyString formal_position) {
    int error = HeadlessInitSolver(game_name, variant_id, data_path);
    if (error != 0) return error;

    const Game *game = GameManagerGetCurrentGame();
    assert(game != NULL);
    bool implements_regular = ImplementsRegularUwapi(game);
    bool implements_tier = ImplementsTierUwapi(game);

    if (!implements_regular && !implements_tier) {
        fprintf(
            stderr,
            "HeadlessQuery: %s does not have a valid UWAPI implementation\n",
            game_name);
        return -2;
    }

    // Either regular or tier UWAPI is implemented.
    if (implements_regular) return QueryRegular(game, formal_position);
    return QueryTier(game, formal_position);
}

int HeadlessGetStart(ReadOnlyString game_name, int variant_id) {
    // TODO
    printf("HeadlessGetStart: unimplemented\n");
    return 1;
}

int HeadlessGetRandom(ReadOnlyString game_name, int variant_id) {
    // TODO
    printf("HeadlessGetRandom: unimplemented\n");
    return 1;
}

// -----------------------------------------------------------------------------

static bool ImplementsBasicUwapi(const Game *game) {
    return game->uwapi != NULL;
}

static bool ImplementsRegularUwapi(const Game *game) {
    if (!ImplementsBasicUwapi(game)) return false;
    if (game->uwapi->FormalPositionToPosition == NULL) return false;
    if (game->uwapi->PositionToFormalPosition == NULL) return false;
    if (game->uwapi->PositionToUwapiPosition == NULL) return false;
    if (game->uwapi->MoveToUwapiMove == NULL) return false;

    return true;
}

static bool ImplementsTierUwapi(const Game *game) {
    if (!ImplementsBasicUwapi(game)) return false;
    if (game->uwapi->FormalPositionToTierPosition == NULL) return false;
    if (game->uwapi->TierPositionToFormalPosition == NULL) return false;
    if (game->uwapi->TierPositionToUwapiPosition == NULL) return false;
    if (game->uwapi->TierMoveToUwapiMove == NULL) return false;

    return true;
}

static int QueryRegular(const Game *game, ReadOnlyString formal_position) {
    Position position = game->uwapi->FormalPositionToPosition(formal_position);
    if (position < 0) {
        fprintf(stderr,
                "QueryRegular: failed to convert formal position [%s] into "
                "position\n",
                formal_position);
        return 2;
    }

    DbProbe probe;
    DbManagerProbeInit(&probe);
    TierPosition tier_position = {.tier = kDefaultTier, .position = position};
    Value value = DbManagerProbeValue(&probe, tier_position);

    // TODO: parse this into the UWAPI json format
    printf("value: %s\n", kValueStrings[(int)value]);
    int remoteness = DbManagerProbeRemoteness(&probe, tier_position);
    printf("remoteness: %d\n", remoteness);
    DbManagerProbeDestroy(&probe);
    return 0;
}

static int QueryTier(const Game *game, ReadOnlyString formal_position) {
    // TODO
}

#ifndef GAMESMANONE_CORE_TYPES_GAMEPLAY_API_H
#define GAMESMANONE_CORE_TYPES_GAMEPLAY_API_H

#include "core/types/gameplay_api/gameplay_api_common.h"
#include "core/types/gameplay_api/gameplay_api_regular.h"
#include "core/types/gameplay_api/gameplay_api_tier.h"

/**
 * @brief GAMESMAN interactive game play API.
 *
 * @details In addition to the common set of API, there are two sets of APIs,
 * one for tier games and one for non-tier games. The game developer should
 * implement exactly one of the two APIs and set the other API to NULL. If
 * neither is fully implemented, the game will be rejected by the gameplay
 * system. Implementing both APIs results in undefined behavior.
 */
typedef struct GameplayApi {
    const GameplayApiCommon *common;
    const GameplayApiRegular *regular;
    const GameplayApiTier *tier;
} GameplayApi;

#endif  // GAMESMANONE_CORE_TYPES_GAMEPLAY_API_H

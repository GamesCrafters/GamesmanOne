/**
 * @file gameplay_api.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief GAMESMAN interactive game play API.
 * @details A GameplayApi defines a set of functions that the GAMESMAN
 * interactive system will use to provide a command line interface for gameplay.
 * All games must implement all required fields in GameplayApi::common and one
 * of the other API collections depending on the type of the game.
 * @version 1.0.0
 * @date 2024-01-22
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
    /** Common API collection. */
    const GameplayApiCommon *common;   

    /** API functions for regular (non-tier) games. */
    const GameplayApiRegular *regular;

    /** API functions for tier games. */
    const GameplayApiTier *tier;       
} GameplayApi;

#endif  // GAMESMANONE_CORE_TYPES_GAMEPLAY_API_H

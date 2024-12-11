/**
 * @file uwapi.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief High-level container of helper methods that are used by games to
 * generate responses for GamesCraftersUWAPI (Universal Web API.)
 * @details A Uwapi object contains a set of helper functions that facilitates
 * the generation of JSON responses for GamesCraftersUWAPI (Universal Web API).
 * UWAPI is an internal request-routing server framework that allows the
 * backend solving and serving systems such as GamesmanOne and GamesmanClassic
 * to provide game rules and database querying service for the GamesmanUni
 * online game generator.
 * @link https://github.com/GamesCrafters/GamesCraftersUWAPI
 * @link https://github.com/GamesCrafters/GamesmanUni
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

#ifndef GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_H_
#define GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_H_

#include "core/types/uwapi/uwapi_regular.h"
#include "core/types/uwapi/uwapi_tier.h"

/**
 * @brief High-level container of helper methods that are used by games to
 * generate responses for GamesCraftersUWAPI (Universal Web API.)
 *
 * @note Depending on the type of the game, the game developer should implement
 * one appropriate API collection and set all other member APIs to NULL.
 * Implementing more than one set of API results in undefined behavior.
 */
typedef struct Uwapi {
    const UwapiRegular *regular; /**< API for regular (non-tier) games. */
    const UwapiTier *tier;       /**< API for tier games. */
} Uwapi;

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_H_

/**
 * @file gameplay_api_tier.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The GameplayApiTier type.
 * @details A GameplayApiTier object contains a set of API functions that all
 * tier games should implement as part of their gameplay API. All member
 * variables and functions of GameplayApiTier are REQUIRED unless otherwise
 * specified.
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

#ifndef GAMESMANONE_CORE_TYPES_GAMEPLAY_API_TIER_H
#define GAMESMANONE_CORE_TYPES_GAMEPLAY_API_TIER_H

#include "core/types/base.h"
#include "core/types/move_array.h"

/**
 * @brief Collection of Gameplay API functions needed by tier games.
 *
 * @note The implementation of all functions are REQUIRED unless otherwise
 * specified.
 */
typedef struct GameplayApiTier {
    /**
     * @brief Returns the tier in which the initial position belongs to.
     * Different game variants may have different initial tiers.
     */
    Tier (*GetInitialTier)(void);

    /**
     * @brief Converts TIER_POSITION into a position string and stores it in
     * BUFFER.
     *
     * @note Assumes that TIER_POSITION is valid and BUFFER has enough space to
     * hold the position string. Results in undefined behavior otherwise.
     */
    int (*TierPositionToString)(TierPosition tier_position, char *buffer);

    /**
     * @brief Returns an array of available moves at the given TIER_POSITION.
     *
     * @note Assumes that TIER_POSITION is valid. Results in undefined behavior
     * otherwise.
     */
    MoveArray (*GenerateMoves)(TierPosition tier_position);

    /**
     * @brief Returns the resulting tier position after performing the given
     * MOVE at the given TIER_POSITION.
     *
     * @note Assumes that TIER_POSITION is valid and MOVE is a valid move at
     * TIER_POSITION. Passing an invalid tier, an illegal position within the
     * tier, or an illegal move results in undefined behavior.
     */
    TierPosition (*DoMove)(TierPosition tier_position, Move move);

    /**
     * @brief Returns the value of the given TIER_POSITION if it is primitive,
     * or kUndecided otherwise.
     *
     * @note Assumes TIER_POSITION is valid. Passing an invalid tier or an
     * illegal position within the tier results in undefined behavior.
     */
    Value (*Primitive)(TierPosition tier_position);
} GameplayApiTier;

#endif  // GAMESMANONE_CORE_TYPES_GAMEPLAY_API_TIER_H

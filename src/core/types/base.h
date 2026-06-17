/**
 * @file base.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief GAMESMAN base types.
 * @version 1.0.2
 * @date 2024-09-07
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

#ifndef GAMESMANONE_CORE_TYPES_BASE_H_
#define GAMESMANONE_CORE_TYPES_BASE_H_

#include <inttypes.h>  // PRId64
#include <stdint.h>    // int64_t

/** @brief Tier as a 64-bit integer. */
typedef int64_t Tier;

#define PRITier PRId64  // Print format macro for the Tier type.

/** @brief Game position as a 64-bit integer hash. */
typedef int64_t Position;

#define PRIPos PRId64  // Print format macro for the Position type.

/** @brief Game move as a 64-bit integer. */
typedef int64_t Move;

#define PRIMove PRId64  // Print format macro for the Move type.

/**
 * @brief Tier and Position. In Tier games, a position is uniquely identified by
 * the Tier it belongs and its Position hash inside that tier.
 */
typedef struct TierPosition {
    Tier tier;
    Position position;
} TierPosition;

/**
 * @brief Possible values of a game position.
 *
 * @details The following conventions must be observed as other components of
 * GAMESMAN rely on them:
 *   1. kUndecided is 0
 *   2. kLose < kDraw < kTie < kWin
 *   3. kNumValues is greater than all other possible values.
 */
typedef enum Value {
    kErrorValue = -1, /**< This illegal value indicates an error. */
    kUndecided = 0,   /**< Value has not been decided. */
    kLose,            /**< Current player is losing in perfect play. */
    kDraw,            /**< Players are in a draw assuming perfect play. */
    kTie,             /**< The game will end in a tie assuming perfect play. */
    kWin,             /**< Current player is winning in perfect play. */
    kNumValues,       /**< Number of possible legal values. */
} Value;

/**
 * @brief Type of a tier.
 */
typedef enum TierType {
    /**
     * @brief A tier T is of this type if for all positions P in T, none of the
     * child positions of P are in T. This also implies that T is loop-free.
     */
    kTierTypeImmediateTransition,

    /**
     * @brief A tier T is of this type if it is loop-free. That is, there are no
     * cycles in the position graph of T.
     */
    kTierTypeLoopFree,

    /**
     * @brief A tier T is of this type if it is loopy or if its loopiness is
     * unclear.
     *
     * @note The loopy algorithm also works on loop-free tiers. Hence, this
     * should be the default type of a tier if its type is not specified.
     */
    kTierTypeLoopy,
} TierType;

/**
 * @brief Pointer to a read-only string cannot be used to modify the contents of
 * the string.
 */
typedef const char *ReadOnlyString;

/**
 * @brief Constant pointer to a read-only string. The pointer is constant, which
 * means its value cannot be changed once it is initialized. The string is
 * read-only, which means that the pointer cannot be used to modify the contents
 * of the string.
 */
typedef const ReadOnlyString ConstantReadOnlyString;

#endif  // GAMESMANONE_CORE_TYPES_BASE_H_

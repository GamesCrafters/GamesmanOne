/**
 * @file position_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing Position hash set.
 *
 * @version 1.0.0
 * @date 2024-01-24
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

#ifndef GAMESMANONE_CORE_TYPES_POSITION_HASH_SET_H
#define GAMESMANONE_CORE_TYPES_POSITION_HASH_SET_H

#include "core/data_structures/int64_hash_map.h"
#include "core/types/base.h"

/** @brief Linear-probing Position hash set using Int64HashMap. */
typedef Int64HashMap PositionHashSet;

/**
 * @brief Initializes Position hash set SET to an empty set with maximum load
 * factor MAX_LOAD_FACTOR.
 *
 * @param set Set to initialize.
 * @param max_load_factor Set maximum load factor of SET to this value. The hash
 * set will automatically expand its capacity if (double)size/capacity is
 * greater than this value. A small max_load_factor trades memory for
 * speed whereas a large max_load_factor trades speed for memory. This value is
 * restricted to be in the range [0.25, 0.75]. If the user passes a
 * max_load_factor that is smaller than 0.25 or greater than 0.75, the internal
 * value will be set to 0.25 and 0.75, respectively, regardless of the
 * user-specified value.
 */
void PositionHashSetInit(PositionHashSet *set, double max_load_factor);

/** @brief Destroys the Position hash set SET. */
void PositionHashSetDestroy(PositionHashSet *set);

/**
 * @brief Returns true if the Position hash set SET contains POSITION, or false
 * otherwise.
 */
bool PositionHashSetContains(PositionHashSet *set, Position position);

/**
 * @brief Adds POSITION to the Position hash set SET.
 *
 * @param set Destination Position hash set.
 * @param position Position to add.
 * @return true on success,
 * @return false otherwise.
 */
bool PositionHashSetAdd(PositionHashSet *set, Position position);

#endif  // GAMESMANONE_CORE_TYPES_POSITION_HASH_SET_H

/**
 * @file position_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamically-sized linear probing Position hash set with sentinel value
 * optimization.
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

#ifndef GAMESMANONE_CORE_TYPES_POSITION_HASH_SET_H_
#define GAMESMANONE_CORE_TYPES_POSITION_HASH_SET_H_

#include <stdbool.h>
#include <stdint.h>

#include "core/data_structures/int64_hash_set.h"
#include "core/types/base.h"

/**
 * @brief A linear-probing Position hash set using `Int64HashSet` as the
 * underlying type. Using `INT64_MIN` as the sentinel value to represent empty
 * slots. The sentinel value must not be inserted or tested as a key or the
 * behavior is undefined.
 */
typedef Int64HashSet PositionHashSet;

/**
 * @brief Initializes the given `set` to an empty set with maximum load
 * factor `max_load_factor`.
 *
 * @param[out] set Set to initialize.
 * @param[in] max_load_factor Set maximum load factor of `set` to this value.
 * The hash set will automatically expand its capacity if (double)size/capacity
 * is greater than `max_load_factor`. A small value trades memory for speed
 * whereas a large value trades speed for memory. This value is restricted to be
 * in the range [0.25, 0.75] to provide optimal performance. The actual max load
 * factor is capped at 0.25 and 0.75 respectively if the user passes a value
 * that is smaller than 0.25 or greater than 0.75.
 */
static inline void PositionHashSetInit(PositionHashSet *set,
                                       double max_load_factor) {
    Int64HashSetInit(set, max_load_factor);
}

/**
 * @brief Attempts to reserve space for `size` `Position`s in `set`. If `true`
 * is returned, the target hash set `set` is guaranteed to have space for at
 * least `size` `Position`s before it expands internally. If `false` is
 * returned, the hash set remains unchanged.
 *
 * @param[in,out] set Target hash set.
 * @param[in] size Number of `Position`s to reserve space for.
 *
 * @retval true Space was successfully reserved.
 * @retval false The hash set remains unchanged.
 */
static inline bool PositionHashSetReserve(PositionHashSet *set, int64_t size) {
    return Int64HashSetReserve(set, size);
}

/**
 * @brief Deallocates the given `set`.
 *
 * @param[in,out] set The `PositionHashSet` to destroy.
 */
static inline void PositionHashSetDestroy(PositionHashSet *set) {
    Int64HashSetDestroy(set);
}

/**
 * @brief Tests if `position` is in `set`.
 *
 * @param[in] set Set from which the given `position` is looked up.
 * @param[in] position `Position` hash value to look for.
 *
 * @retval true The `set` contains `position`.
 * @retval false The `set` does not contain `position`.
 */
static inline bool PositionHashSetContains(PositionHashSet *set,
                                           Position position) {
    return Int64HashSetContains(set, position);
}

/**
 * @brief Adds `position` to the given `set` or does nothing if `set` already
 * contains `position`.
 *
 * @param[in,out] set Set to add `position` into.
 * @param[in] position `Position` to be added to `set`.
 *
 * @retval true The `position` was added into `set` as a new key.
 * @retval false The `set` already contains `position` or an error occurred.
 */
static inline bool PositionHashSetAdd(PositionHashSet *set, Position position) {
    return Int64HashSetAdd(set, position);
}

#endif  // GAMESMANONE_CORE_TYPES_POSITION_HASH_SET_H_

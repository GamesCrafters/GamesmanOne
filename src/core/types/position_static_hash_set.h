/**
 * @file position_static_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-capacity linear-probing Position hash set with sentinel
 * value optimization.
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

#ifndef GAMESMANONE_CORE_TYPES_POSITION_STATIC_HASH_SET_H_
#define GAMESMANONE_CORE_TYPES_POSITION_STATIC_HASH_SET_H_

#include <stdbool.h>

#include "core/data_structures/int64_static_hash_set.h"
#include "core/types/base.h"

/**
 * @brief Fixed-capacity linear-probing Position hash set using
 * Int64StaticHashSet as underlying type.
 */
typedef Int64StaticHashSet PositionStaticHashSet;

#define POSITION_STATIC_HASH_SET_EMPTY_KEY INT64_STATIC_HASH_SET_EMPTY_KEY

/**
 * @brief Macro helper to declare and initialize a stack-allocated
 * `PositionStaticHashSet` instance along with its underlying key array.
 *
 * @param[out] name Name of the `PositionStaticHashSet` variable to create.
 * @param[in] cap Capacity of the hash set; must be a compile-time constant
 * positive integral value and a power of 2.
 */
#define DECLARE_POSITION_STATIC_HASH_SET(name, cap) \
    DECLARE_INT64_STATIC_HASH_SET(name, cap)

/**
 * @brief Adds a `key` to the static hash set.
 *
 * @param[in,out] set The `PositionStaticHashSet` to modify.
 * @param[in] key The value to add.
 *
 * @retval true The `key` was successfully inserted into the set.
 * @retval false The `key` already exists in the set.
 */
static inline bool PositionStaticHashSetAdd(PositionStaticHashSet *set,
                                            Position key) {
    return Int64StaticHashSetAdd(set, key);
}

/**
 * @brief Returns the number of elements in the static hash set.
 *
 * @param[in] set The `PositionStaticHashSet` to inspect.
 *
 * @return The number of elements in `set`.
 */
static inline int PositionStaticHashSetGetSize(
    const PositionStaticHashSet *set) {
    return Int64StaticHashSetGetSize(set);
}

#endif  // GAMESMANONE_CORE_TYPES_POSITION_STATIC_HASH_SET_H_

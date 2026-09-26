/**
 * @file tier_hash_map.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing `Tier` hash map that maps `Tier`s to 64-bit signed
 * integers.
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_H_
#define GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_H_

#include <stdbool.h>
#include <stdint.h>

#include "core/data_structures/int64_hash_map.h"
#include "core/gamesman_memory.h"
#include "core/types/base.h"

/**
 * @brief Linear-probing `Tier` to `int64_t` hash map using `Int64HashMap`.
 *
 * @warning This typedef should be treated as opaque. Use accessor and mutator
 * functions to interact with the hash map.
 */
typedef Int64HashMap TierHashMap;

/**
 * @brief Iterator for `TierHashMap`.
 *
 * @warning This typedef should be treated as opaque. Use accessor and mutator
 * functions to interact with the iterator.
 */
typedef Int64HashMapIterator TierHashMapIterator;

/**
 * @brief Initializes the given `map`.
 *
 * @details No memory is allocated until the first insertion or reservation.
 * The `max_load_factor` is clamped to [0.5, 0.8].
 *
 * @param[out] map The `TierHashMap` to initialize.
 * @param[in] max_load_factor The maximum load factor, clamped to [0.5, 0.8].
 */
static inline void TierHashMapInit(TierHashMap *map, double max_load_factor) {
    Int64HashMapInit(map, max_load_factor);
}

/**
 * @brief Initializes the given `map` using the given memory `allocator`.
 *
 * @details No memory is allocated until the first insertion or reservation.
 * The `max_load_factor` is clamped to [0.5, 0.8]. If `allocator` is `NULL`,
 * the effect is equivalent to calling `TierHashMapInit`.
 *
 * @param[out] map The `TierHashMap` to initialize.
 * @param[in] max_load_factor The maximum load factor, clamped to [0.5, 0.8].
 * @param[in] allocator Memory allocator to use, or `NULL` for default.
 */
static inline void TierHashMapInitAllocator(TierHashMap *map,
                                            double max_load_factor,
                                            GamesmanAllocator *allocator) {
    Int64HashMapInitAllocator(map, max_load_factor, allocator);
}

/**
 * @brief Frees the memory associated with the hash map and resets its state.
 *
 * @param[in,out] map The `TierHashMap` to destroy.
 */
static inline void TierHashMapDestroy(TierHashMap *map) {
    Int64HashMapDestroy(map);
}

/**
 * @brief Returns an iterator pointing to the entry with the given `key` in
 * `map`, or an invalid iterator if `key` is not found.
 *
 * @details Use `TierHashMapIteratorIsValid` to check whether `key` was found.
 *
 * @param[in] map The `TierHashMap` to search.
 * @param[in] key The key to search for.
 *
 * @return A `TierHashMapIterator` pointing to the entry with `key`, or an
 * invalid iterator if `key` is not found.
 */
static inline TierHashMapIterator TierHashMapGet(TierHashMap *map, Tier key) {
    return Int64HashMapGet(map, key);
}

/**
 * @brief Sets the value associated with `tier` in `map` to `value`.
 *
 * @details Creates a new entry if `tier` does not exist in `map`. If `tier`
 * already exists, its value is updated to `value`. If the operation fails for
 * any reason, `map` remains unchanged.
 *
 * @param[in,out] map The `TierHashMap` to modify.
 * @param[in] tier The key of the entry.
 * @param[in] value The value to associate with `tier`.
 *
 * @retval true The entry was successfully set.
 * @retval false Memory allocation failed during expansion.
 */
static inline bool TierHashMapSet(TierHashMap *map, Tier tier, int64_t value) {
    return Int64HashMapSet(map, tier, value);
}

/**
 * @brief Returns whether `map` contains an entry with the given `tier`.
 *
 * @param[in] map The `TierHashMap` to search.
 * @param[in] tier The key to search for.
 *
 * @retval true The `tier` is present in the map.
 * @retval false The `tier` is not present, or the map is not initialized.
 */
static inline bool TierHashMapContains(const TierHashMap *map, Tier tier) {
    return Int64HashMapContains(map, tier);
}

/**
 * @brief Returns an iterator positioned before the first entry of `map`.
 *
 * @details This function is designed to be used with `TierHashMapIteratorNext`
 * to iterate through all entries in the hash map.
 *
 * @param[in] map The `TierHashMap` to iterate over.
 *
 * @return A `TierHashMapIterator` positioned before the first entry.
 */
static inline TierHashMapIterator TierHashMapBegin(TierHashMap *map) {
    return Int64HashMapBegin(map);
}

/**
 * @brief Returns the key of the entry that `it` is pointing to.
 *
 * @warning Calling this function on an invalid iterator results in undefined
 * behavior. Use `TierHashMapIteratorIsValid` to validate the iterator first.
 *
 * @param[in] it The `TierHashMapIterator` to read.
 *
 * @return The key of the entry.
 */
static inline Tier TierHashMapIteratorKey(const TierHashMapIterator *it) {
    return Int64HashMapIteratorKey(it);
}

/**
 * @brief Returns the value of the entry that `it` is pointing to.
 *
 * @warning Calling this function on an invalid iterator results in undefined
 * behavior. Use `TierHashMapIteratorIsValid` to validate the iterator first.
 *
 * @param[in] it The `TierHashMapIterator` to read.
 *
 * @return The value of the entry.
 */
static inline int64_t TierHashMapIteratorValue(const TierHashMapIterator *it) {
    return Int64HashMapIteratorValue(it);
}

/**
 * @brief Returns whether the given iterator `it` is valid.
 *
 * @param[in] it The `TierHashMapIterator` to validate.
 *
 * @retval true The iterator points to a valid entry.
 * @retval false The iterator is invalid.
 */
static inline bool TierHashMapIteratorIsValid(const TierHashMapIterator *it) {
    return Int64HashMapIteratorIsValid(it);
}

/**
 * @brief Advances `iterator` to the next valid entry and returns `true`, or
 * returns `false` if no more entries exist.
 *
 * @details This function is designed to be used with `TierHashMapBegin` to
 * iterate through all entries in the hash map.
 *
 * @param[in,out] iterator The `TierHashMapIterator` to advance.
 * @param[out] tier Pointer to receive the key, or `NULL` if not needed.
 * @param[out] value Pointer to receive the value, or `NULL` if not needed.
 *
 * @retval true The iterator was advanced to a valid entry.
 * @retval false No more entries exist.
 */
static inline bool TierHashMapIteratorNext(TierHashMapIterator *iterator,
                                           Tier *tier, int64_t *value) {
    return Int64HashMapIteratorNext(iterator, tier, value);
}

#endif  // GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_H_

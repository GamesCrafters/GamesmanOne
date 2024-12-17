/**
 * @file tier_hash_map.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of linear-probing Tier hash map.
 * @version 1.0.1
 * @date 2024-09-02
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

#include "core/types/tier_hash_map.h"

#include <stdint.h>  // int64_t

#include "core/data_structures/int64_hash_map.h"
#include "core/types/base.h"

void TierHashMapInit(TierHashMap *map, double max_load_factor) {
    Int64HashMapInit(map, max_load_factor);
}

void TierHashMapDestroy(TierHashMap *map) { Int64HashMapDestroy(map); }

TierHashMapIterator TierHashMapGet(TierHashMap *map, Tier key) {
    return Int64HashMapGet(map, key);
}

bool TierHashMapSet(TierHashMap *map, Tier tier, int64_t value) {
    return Int64HashMapSet(map, tier, value);
}

bool TierHashMapContains(const TierHashMap *map, Tier tier) {
    return Int64HashMapContains(map, tier);
}

TierHashMapIterator TierHashMapBegin(TierHashMap *map) {
    return Int64HashMapBegin(map);
}

Tier TierHashMapIteratorKey(const TierHashMapIterator *it) {
    return Int64HashMapIteratorKey(it);
}

int64_t TierHashMapIteratorValue(const TierHashMapIterator *it) {
    return Int64HashMapIteratorValue(it);
}

bool TierHashMapIteratorIsValid(const TierHashMapIterator *it) {
    return Int64HashMapIteratorIsValid(it);
}

bool TierHashMapIteratorNext(TierHashMapIterator *iterator, Tier *tier,
                             int64_t *value) {
    return Int64HashMapIteratorNext(iterator, tier, value);
}

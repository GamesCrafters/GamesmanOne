/**
 * @file tier_to_ptr_chained_hash_map.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Separate chaining Tier to generic pointer (void *) hash map.
 * implementation.
 * @version 1.0.0
 * @date 2025-06-09
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

#include "core/types/tier_to_ptr_chained_hash_map.h"

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/data_structures/int64_to_ptr_chained_hash_map.h"
#include "core/types/base.h"

void TierToPtrChainedHashMapInit(TierToPtrChainedHashMap *map,
                                 double max_load_factor) {
    Int64ToPtrChainedHashMapInit(map, max_load_factor);
}

void TierToPtrChainedHashMapDestroy(TierToPtrChainedHashMap *map) {
    Int64ToPtrChainedHashMapDestroy(map);
}

bool TierToPtrChainedHashMapContains(const TierToPtrChainedHashMap *map,
                                     Tier tier) {
    return Int64ToPtrChainedHashMapContains(map, tier);
}

TierToPtrChainedHashMapIterator TierToPtrChainedHashMapGet(
    const TierToPtrChainedHashMap *map, Tier tier) {
    return Int64ToPtrChainedHashMapGet(map, tier);
}

bool TierToPtrChainedHashMapSet(TierToPtrChainedHashMap *map, Tier tier,
                                void *value) {
    return Int64ToPtrChainedHashMapSet(map, tier, value);
}

void TierToPtrChainedHashMapRemove(TierToPtrChainedHashMap *map, Tier tier) {
    Int64ToPtrChainedHashMapRemove(map, tier);
}

TierToPtrChainedHashMapIterator TierToPtrChainedHashMapBegin(
    const TierToPtrChainedHashMap *map) {
    return Int64ToPtrChainedHashMapBegin(map);
}

int64_t TierToPtrChainedHashMapIteratorKey(
    const TierToPtrChainedHashMapIterator *it) {
    return Int64ToPtrChainedHashMapIteratorKey(it);
}

void *TierToPtrChainedHashMapIteratorValue(
    const TierToPtrChainedHashMapIterator *it) {
    return Int64ToPtrChainedHashMapIteratorValue(it);
}

bool TierToPtrChainedHashMapIteratorIsValid(
    const TierToPtrChainedHashMapIterator *it) {
    return Int64ToPtrChainedHashMapIteratorIsValid(it);
}

bool TierToPtrChainedHashMapIteratorNext(TierToPtrChainedHashMapIterator *it) {
    return Int64ToPtrChainedHashMapIteratorNext(it);
}

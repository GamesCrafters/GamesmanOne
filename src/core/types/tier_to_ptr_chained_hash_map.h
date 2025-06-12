/**
 * @file tier_to_ptr_chained_hash_map.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Separate chaining Tier to generic pointer (void *) hash map.
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_TO_PTR_CHAINED_HASH_MAP_H_
#define GAMESMANONE_CORE_TYPES_TIER_TO_PTR_CHAINED_HASH_MAP_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/data_structures/int64_to_ptr_chained_hash_map.h"
#include "core/types/base.h"

typedef Int64ToPtrChainedHashMap TierToPtrChainedHashMap;

typedef struct Int64ToPtrChainedHashMapIterator TierToPtrChainedHashMapIterator;

void TierToPtrChainedHashMapInit(TierToPtrChainedHashMap *map,
                                 double max_load_factor);

void TierToPtrChainedHashMapDestroy(TierToPtrChainedHashMap *map);

bool TierToPtrChainedHashMapContains(const TierToPtrChainedHashMap *map,
                                     Tier tier);

TierToPtrChainedHashMapIterator TierToPtrChainedHashMapGet(
    const TierToPtrChainedHashMap *map, Tier tier);

bool TierToPtrChainedHashMapSet(TierToPtrChainedHashMap *map, Tier tier,
                                void *value);

void TierToPtrChainedHashMapRemove(TierToPtrChainedHashMap *map, Tier tier);

TierToPtrChainedHashMapIterator TierToPtrChainedHashMapBegin(
    const TierToPtrChainedHashMap *map);

int64_t TierToPtrChainedHashMapIteratorKey(
    const TierToPtrChainedHashMapIterator *it);

void *TierToPtrChainedHashMapIteratorValue(
    const TierToPtrChainedHashMapIterator *it);

bool TierToPtrChainedHashMapIteratorIsValid(
    const TierToPtrChainedHashMapIterator *it);

bool TierToPtrChainedHashMapIteratorNext(TierToPtrChainedHashMapIterator *it);

#endif  // GAMESMANONE_CORE_TYPES_TIER_TO_PTR_CHAINED_HASH_MAP_H_

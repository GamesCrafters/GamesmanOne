/**
 * @file tier_hash_map_sc.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Separate chaining Tier to int64_t hash map implementation.
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

#include "core/types/tier_hash_map_sc.h"

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/data_structures/int64_hash_map_sc.h"
#include "core/types/base.h"

void TierHashMapSCInit(TierHashMapSC *map, double max_load_factor) {
    Int64HashMapSCInit(map, max_load_factor);
}

void TierHashMapSCDestroy(TierHashMapSC *map) { Int64HashMapSCDestroy(map); }

bool TierHashMapSCContains(const TierHashMapSC *map, Tier tier) {
    return Int64HashMapSCContains(map, tier);
}

bool TierHashMapSCGet(const TierHashMapSC *map, Tier tier, int64_t *value) {
    return Int64HashMapSCGet(map, tier, value);
}

bool TierHashMapSCSet(TierHashMapSC *map, Tier tier, int64_t value) {
    return Int64HashMapSCSet(map, tier, value);
}

void TierHashMapSCRemove(TierHashMapSC *map, Tier tier) {
    Int64HashMapSCRemove(map, tier);
}

/**
 * @file tier_position_hash_set.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing TierPosition hash set implementation.
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

#include "core/types/tier_position_hash_set.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/data_structures/hash.h"
#include "core/gamesman_memory.h"
#include "core/types/base.h"

bool TierPositionHashSetInternalExpand(TierPositionHashSet *set) {
    // If keys is non-NULL, this is a normal expansion step;
    // if keys is NULL, this is the lazy initialization step.
    // Initial capacity is 128, so the mask 127 (0x7F).
    uint64_t new_mask = set->keys ? ((set->mask << 1) | 1ULL) : 0x7F;

    return TierPositionHashSetInternalExpandExplicit(set, new_mask);
}

bool TierPositionHashSetInternalExpandExplicit(TierPositionHashSet *set,
                                               uint64_t new_mask) {
    // Allocate new array and initialize it
    TierPosition *__restrict new_keys =
        (TierPosition *)GamesmanMalloc((new_mask + 1) * sizeof(TierPosition));
    if (new_keys == NULL) {
        return false;
    }
    for (uint64_t i = 0; i <= new_mask; ++i) {
        new_keys[i].tier = TIER_POSITION_HASH_SET_EMPTY_TIER;
    }

    // Hoist pointer to local so the compiler doesn't worry about memory
    // aliasing during the loop.
    TierPosition *__restrict old_keys = set->keys;

    // Only attempt to rehash if we had existing keys
    if (old_keys != NULL) {
        uint64_t old_mask = set->mask;
        for (uint64_t i = 0; i <= old_mask; ++i) {
            TierPosition key = old_keys[i];
            // We only need to check the tier to see if the slot is empty
            if (key.tier != TIER_POSITION_HASH_SET_EMPTY_TIER) {
                uint64_t new_index =
                    Hash128to64((int64_t)key.tier, (int64_t)key.position) &
                    new_mask;
                while (new_keys[new_index].tier !=
                       TIER_POSITION_HASH_SET_EMPTY_TIER) {
                    new_index = (new_index + 1) & new_mask;
                }
                new_keys[new_index] = key;
            }
        }
    }

    // Update internal data
    GamesmanFree(old_keys);
    set->keys = new_keys;
    set->mask = new_mask;
    set->max_size = (int64_t)((new_mask + 1) / set->inv_max_load_factor);

    return true;
}

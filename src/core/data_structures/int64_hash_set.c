#include "core/data_structures/int64_hash_set.h"

#include <stdbool.h>
#include <stdint.h>

#include "core/data_structures/hash.h"
#include "core/gamesman_memory.h"

bool Int64HashSetInternalExpand(Int64HashSet *set) {
    // If old_keys is non-NULL, this is a normal expansion step;
    // if old_keys in NULL, this is the lazy initialization step.
    // Initial capacity is 16, so the mask is 0xF.
    uint64_t new_mask = set->keys ? ((set->mask << 1) | 1ULL) : 0xFULL;

    return Int64HashSetInternalExpandExplicit(set, new_mask);
}

bool Int64HashSetInternalExpandExplicit(Int64HashSet *set, uint64_t new_mask) {
    // Allocate new array and initialize it
    int64_t *__restrict new_keys =
        (int64_t *)GamesmanMalloc((new_mask + 1) * sizeof(int64_t));
    if (new_keys == NULL) {
        return false;
    }
    for (uint64_t i = 0; i <= new_mask; ++i) {
        new_keys[i] = INT64_HASH_SET_EMPTY_KEY;
    }

    // Hoist pointer to local so the compiler doesn't worry about memory
    // aliasing during the loop.
    int64_t *__restrict old_keys = set->keys;

    // Only attempt to rehash if we had existing keys
    if (old_keys != NULL) {
        uint64_t old_mask = set->mask;
        for (uint64_t i = 0; i <= old_mask; ++i) {
            int64_t key = old_keys[i];
            if (key != INT64_HASH_SET_EMPTY_KEY) {
                uint64_t new_index = Splitmix64(key) & new_mask;
                while (new_keys[new_index] != INT64_HASH_SET_EMPTY_KEY) {
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

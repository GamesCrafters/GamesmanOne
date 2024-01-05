#include "core/types/position_hash_set.h"

#include "core/data_structures/int64_hash_map.h"
#include "core/types/base.h"

void PositionHashSetInit(PositionHashSet *set, double max_load_factor) {
    Int64HashMapInit(set, max_load_factor);
}

void PositionHashSetDestroy(PositionHashSet *set) { Int64HashMapDestroy(set); }

bool PositionHashSetContains(PositionHashSet *set, Position position) {
    return Int64HashMapContains(set, position);
}

bool PositionHashSetAdd(PositionHashSet *set, Position position) {
    return Int64HashMapSet(set, position, 0);
}

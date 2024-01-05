#ifndef GAMESMANONE_CORE_TYPES_POSITION_HASH_SET_H
#define GAMESMANONE_CORE_TYPES_POSITION_HASH_SET_H

#include "core/data_structures/int64_hash_map.h"
#include "core/types/base.h"

/** @brief Linear-probing Position hash set using Int64HashMap. */
typedef Int64HashMap PositionHashSet;

void PositionHashSetInit(PositionHashSet *set, double max_load_factor);
void PositionHashSetDestroy(PositionHashSet *set);
bool PositionHashSetContains(PositionHashSet *set, Position position);
bool PositionHashSetAdd(PositionHashSet *set, Position position);

#endif  // GAMESMANONE_CORE_TYPES_POSITION_HASH_SET_H

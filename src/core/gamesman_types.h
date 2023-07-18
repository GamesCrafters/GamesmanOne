#ifndef GAMESMANEXPERIMENT_CORE_GAMESMAN_TYPES_H_
#define GAMESMANEXPERIMENT_CORE_GAMESMAN_TYPES_H_

#include <stdint.h>

#include "core/data_structures/int64_array.h"
#include "core/data_structures/int64_queue.h"
#include "core/data_structures/int64_hash_map.h"

typedef int64_t Position;
typedef int64_t Move;

// Always make sure that kUndecided is 0.
typedef enum { kUndecided, kLose, kDraw, kTie, kWin } Value;

typedef Int64Array PositionArray;

typedef Int64Array MoveArray;

typedef int64_t Tier;
typedef Int64Array TierArray;
typedef Int64Array TierStack;
typedef Int64Queue TierQueue;
typedef Int64HashMap TierHashMap;
typedef Int64HashMapIterator TierHashMapIterator;

typedef struct {
    Tier tier;
    Position position;
} TierPosition;

#endif  // GAMESMANEXPERIMENT_CORE_GAMESMAN_TYPES_H_

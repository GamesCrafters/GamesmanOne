#ifndef GAMESMANEXPERIMENT_CORE_GAMESMAN_TYPES_H_
#define GAMESMANEXPERIMENT_CORE_GAMESMAN_TYPES_H_

#include <stdint.h>

#include "int64_array.h"

typedef int64_t Position;
typedef int64_t Move;
typedef enum { kUndecided, kLose, kDraw, kTie, kWin } Value;

typedef struct MoveListItem {
    Move move;
    struct MoveListItem *next;
} MoveListItem;
typedef struct MoveListItem* MoveList;

typedef Int64Array PositionArray;

typedef int64_t Tier;
typedef Int64Array TierArray;
typedef Int64Array TierStack;

typedef struct {
    Tier tier;
    Position position;
} TierPosition;

#endif  // GAMESMANEXPERIMENT_CORE_GAMESMAN_TYPES_H_

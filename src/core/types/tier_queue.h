#ifndef GAMESMANONE_CORE_TYPES_TIER_QUEUE_H
#define GAMESMANONE_CORE_TYPES_TIER_QUEUE_H

#include "core/data_structures/int64_queue.h"
#include "core/types/base.h"

/** @brief Dynamic Tier queue using Int64Queue. */
typedef Int64Queue TierQueue;

void TierQueueInit(TierQueue *queue);
void TierQueueDestroy(TierQueue *queue);
bool TierQueueIsEmpty(const TierQueue *queue);
int64_t TierQueueSize(const TierQueue *queue);
bool TierQueuePush(TierQueue *queue, Tier tier);
Tier TierQueuePop(TierQueue *queue);

#endif  // GAMESMANONE_CORE_TYPES_TIER_QUEUE_H

#ifndef GAMESMANONE_CORE_TYPES_TIER_STACK_H
#define GAMESMANONE_CORE_TYPES_TIER_STACK_H

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

/** @brief Dynamic Tier stack using Int64Array. */
typedef Int64Array TierStack;

void TierStackInit(TierStack *stack);
void TierStackDestroy(TierStack *stack);
bool TierStackPush(TierStack *stack, Tier tier);
void TierStackPop(TierStack *stack);
Tier TierStackTop(const TierStack *stack);
bool TierStackEmpty(const TierStack *stack);

#endif  // GAMESMANONE_CORE_TYPES_TIER_STACK_H

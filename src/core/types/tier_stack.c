#include "core/types/tier_stack.h"

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

void TierStackInit(TierStack *stack) { Int64ArrayInit(stack); }

void TierStackDestroy(TierStack *stack) { Int64ArrayDestroy(stack); }

bool TierStackPush(TierStack *stack, Tier tier) {
    return Int64ArrayPushBack(stack, tier);
}

void TierStackPop(TierStack *stack) { Int64ArrayPopBack(stack); }

Tier TierStackTop(const TierStack *stack) { return Int64ArrayBack(stack); }

bool TierStackEmpty(const TierStack *stack) { return Int64ArrayEmpty(stack); }

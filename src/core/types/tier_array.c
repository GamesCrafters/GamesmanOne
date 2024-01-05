#include "core/types/tier_array.h"

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

void TierArrayInit(TierArray *array) { Int64ArrayInit(array); }

void TierArrayDestroy(TierArray *array) { Int64ArrayDestroy(array); }

bool TierArrayAppend(TierArray *array, Tier tier) {
    return Int64ArrayPushBack(array, tier);
}

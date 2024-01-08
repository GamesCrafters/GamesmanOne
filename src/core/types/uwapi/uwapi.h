#ifndef GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_H
#define GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_H

#include "core/types/uwapi/uwapi_regular.h"
#include "core/types/uwapi/uwapi_tier.h"

typedef struct Uwapi {
    const UwapiRegular *regular;
    const UwapiTier *tier;
} Uwapi;

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_H

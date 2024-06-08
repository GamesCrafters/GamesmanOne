#include "core/db/bpdb/bpdb_lite.h"

#include <stddef.h>  // NULL

#include "core/types/gamesman_types.h"

// Database API.

const Database kBpdbLite = {
    .name = "bpdb_lite",
    .formal_name = "Bit-Perfect Database",

    .Init = NULL,
    .Finalize = NULL,

    // Solving
    .CreateSolvingTier = NULL,
    .FlushSolvingTier = NULL,
    .FreeSolvingTier = NULL,
    .SetValue = NULL,
    .SetRemoteness = NULL,
    .GetValue = NULL,
    .GetRemoteness = NULL,

    // Probing
    .ProbeInit = NULL,
    .ProbeDestroy = NULL,
    .ProbeValue = NULL,
    .ProbeRemoteness = NULL,
    .TierStatus = NULL,
};

#ifndef GAMESMANEXPERIMENT_CORE_DB_MANAGER_H_
#define GAMESMANEXPERIMENT_CORE_DB_MANAGER_H_

#include <stdint.h>  // int64_t

#include "core/gamesman_types.h"

int DbManagerInitDb(const Solver *solver);
void DbManagerFinalizeDb(void);

// Solving Interface.
int DbManagerCreateSolvingTier(Tier tier, int64_t size);
int DbManagerFlushSolvingTier(void *aux);
int DbManagerFreeSolvingTier(void);
int DbManagerSetValue(Position position, Value value);
int DbManagerSetRemoteness(Position position, int remoteness);
Value DbManagerGetValue(Position position);
int DbManagerGetRemoteness(Position position);

// Probing Interface.
int DbManagerProbeInit(DbProbe *probe);
int DbManagerProbeDestroy(DbProbe *probe);
Value DbManagerProbeValue(DbProbe *probe, TierPosition tier_position);
int DbManagerProbeRemoteness(DbProbe *probe, TierPosition tier_position);

#endif  // GAMESMANEXPERIMENT_CORE_DB_MANAGER_H_

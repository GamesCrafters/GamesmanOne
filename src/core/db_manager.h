#ifndef GAMESMANEXPERIMENT_CORE_DB_MANAGER_H_
#define GAMESMANEXPERIMENT_CORE_DB_MANAGER_H_

#include "core/gamesman_types.h"

int DbManagerInitDb(const Solver *solver);

int DbManagerSetValue(Position position, Value value);
int DbManagerSetRemoteness(Position position, int remoteness);
Value DbManagerGetValue(TierPosition tier_position);
int DbManagerGetRemoteness(TierPosition tier_position);

#endif  // GAMESMANEXPERIMENT_CORE_DB_MANAGER_H_

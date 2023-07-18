#ifndef GAMESMANEXPERIMENT_CORE_NAIVEDB_H_
#define GAMESMANEXPERIMENT_CORE_NAIVEDB_H_

#include <stdbool.h>

#include "core/gamesman_types.h"

bool DbCreateTier(Tier tier);
bool DbLoadTier(Tier tier);
void DbSave(Tier tier);

Value DbGetValue(Position position);
int DbGetRemoteness(Position position);
void DbSetValueRemoteness(Position position, Value value, int remoteness);
void DbDumpTierAnalysisToGlobal(void);

#endif  // GAMESMANEXPERIMENT_CORE_NAIVEDB_H_

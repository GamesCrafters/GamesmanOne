#ifndef GAMESMANEXPERIMENT_CORE_DB_BPDB_BPDB_PROBE_H_
#define GAMESMANEXPERIMENT_CORE_DB_BPDB_BPDB_PROBE_H_

#include "core/gamesman_types.h"

int BpdbProbeInit(DbProbe *probe);
int BpdbProbeDestroy(DbProbe *probe);
uint64_t BpdbProbeRecord(ConstantReadOnlyString current_path, DbProbe *probe,
                         TierPosition tier_position);

#endif  // GAMESMANEXPERIMENT_CORE_DB_BPDB_BPDB_PROBE_H_

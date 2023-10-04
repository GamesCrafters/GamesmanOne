#ifndef GAMESMANEXPERIMENT_CORE_ANALYSIS_STAT_MANAGER_H_
#define GAMESMANEXPERIMENT_CORE_ANALYSIS_STAT_MANAGER_H_

#include "core/analysis/analysis.h"
#include "core/data_structures/bitstream.h"
#include "core/gamesman_types.h"

int StatManagerInit(ReadOnlyString game_name, int variant);
void StatManagerFinalize(void);

int StatManagerGetStatus(Tier tier);
int StatManagerSaveAnalysis(Tier tier, const Analysis *analysis);
int StatManagerLoadAnalysis(Analysis *dest, Tier tier);

BitStream StatManagerLoadDiscoveryMap(Tier tier);
int StatManagerSaveDiscoveryMap(const BitStream *stream, Tier tier);

#endif  // GAMESMANEXPERIMENT_CORE_ANALYSIS_STAT_MANAGER_H_

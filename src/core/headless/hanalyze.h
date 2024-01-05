#ifndef GAMESMANONE_CORE_HEADLESS_HANALYZE_H_
#define GAMESMANONE_CORE_HEADLESS_HANALYZE_H_

#include <stdbool.h>  // bool

#include "core/types/gamesman_types.h"

int HeadlessAnalyze(ReadOnlyString game_name, int variant_id,
                    ReadOnlyString data_path, bool force, int verbose);

#endif  // GAMESMANONE_CORE_HEADLESS_HANALYZE_H_

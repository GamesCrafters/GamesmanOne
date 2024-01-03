#ifndef GAMESMANONE_CORE_HEADLESS_HSOLVE_H_
#define GAMESMANONE_CORE_HEADLESS_HSOLVE_H_

#include <stdbool.h>  // bool

#include "core/gamesman_types.h"

int HeadlessSolve(ReadOnlyString game_name, int variant_id,
                  ReadOnlyString data_path, bool force, int verbose);

#endif  // GAMESMANONE_CORE_HEADLESS_HSOLVE_H_

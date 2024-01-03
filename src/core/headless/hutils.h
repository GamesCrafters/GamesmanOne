#ifndef GAMESMANONE_CORE_HEADLESS_HUTILS_H_
#define GAMESMANONE_CORE_HEADLESS_HUTILS_H_

#include <stdbool.h>  // bool

#include "core/gamesman_types.h"

int HeadlessGetVerbosity(bool verbose, bool quiet);

int HeadlessRedirectOutput(ReadOnlyString output);

const Game *HeadlessGetGame(ReadOnlyString game_name);

int HeadlessInitSolver(ReadOnlyString game_name, int variant_id,
                       ReadOnlyString data_path);

#endif  // GAMESMANONE_CORE_HEADLESS_HUTILS_H_

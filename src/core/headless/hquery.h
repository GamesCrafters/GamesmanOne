#ifndef GAMESMANONE_CORE_HEADLESS_HQUERY_H_
#define GAMESMANONE_CORE_HEADLESS_HQUERY_H_

#include "core/gamesman_types.h"

int HeadlessQuery(ReadOnlyString game_name, int variant_id,
                  ReadOnlyString data_path, ReadOnlyString formal_position);

int HeadlessGetStart(ReadOnlyString game_name, int variant_id);

int HeadlessGetRandom(ReadOnlyString game_name, int variant_id);

#endif  // GAMESMANONE_CORE_HEADLESS_HQUERY_H_

#include "games/game_list.h"

#include <stddef.h>  // NULL

#include "core/types/gamesman_types.h"

// 1. To add a new game, include the game header here.

#include "games/mallqueenschess/mallqueenschess.h"
#include "games/mttt/mttt.h"
#include "games/mtttier/mtttier.h"

// 2. Then add the new game object to the list. Note that the list must be
// NULL-terminated.

const Game *const kAllGames[] = {&kMtttier, &kMttt, &kMallqueenschess, NULL};

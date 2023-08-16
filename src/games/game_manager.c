#include "games/game_manager.h"

#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL

#include "games/mtttier/mtttier.h"

static const Game *const kAllGames[] = {
    &kMtttier, NULL
};

const Game *const *GameManagerGetAllGames(void) {
    return kAllGames;
}

int GameManagerNumGames(void) {
    static int count = -1;
    if (count >= 0) return count;
    int i = 0;
    while (kAllGames[i] != NULL) {
        ++i;
    }
    return count = i;
}

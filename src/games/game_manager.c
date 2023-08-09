#include "games/game_manager.h"

#include <stddef.h>

#include "games/mttt/mttt.h"
#include "games/mtttier/mtttier.h"

static const Game kAllGames[] = {
    // TODO
};

/*
static const GamesmanGame kAllGames[] = {
    {.name = "mttt", .formal_name = "Tic-Tac-Toe", .Init = &MtttInit},

    {.name = "mtttier",
     .formal_name = "Tic-Tac-Toe (Tier)",
     .Init = &MtttierInit}};
*/

const Game *GameManagerGetAllGames(void) {
    return kAllGames;
}

int GameManagerNumGames(void) {
    static int count = -1;
    if (count >= 0) return count;
    int i = 0;
    while (kAllGames[i].solver_api != NULL) {
        ++i;
    }
    return count = i;
}

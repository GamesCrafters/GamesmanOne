#ifndef GAMESMANEXPERIMENT_GAMES_GAME_MANAGER_H_
#define GAMESMANEXPERIMENT_GAMES_GAME_MANAGER_H_

#include "core/gamesman_types.h"

const Game *const *GameManagerGetAllGames(void);

int GameManagerNumGames(void);

#endif  // GAMESMANEXPERIMENT_GAMES_GAME_MANAGER_H_

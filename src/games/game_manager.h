/**
 * @file game_manager.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The Game Manager Module, which provides a list to all games in
 * GAMESMAN.
 *
 * @version 1.0
 * @date 2023-08-19
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GAMESMANEXPERIMENT_GAMES_GAME_MANAGER_H_
#define GAMESMANEXPERIMENT_GAMES_GAME_MANAGER_H_

#include "core/gamesman_types.h"

/**
 * @brief Returns a NULL-terminated read-only array of all games in GAMESMAN.
 */
const Game *const *GameManagerGetAllGames(void);

/** @brief Returns the total number of games in GAMESMAN. */
int GameManagerNumGames(void);

#endif  // GAMESMANEXPERIMENT_GAMES_GAME_MANAGER_H_

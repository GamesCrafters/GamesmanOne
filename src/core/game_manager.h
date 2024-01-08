/**
 * @file game_manager.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The Game Manager Module which handles game initialization and
 * finalization.
 *
 * @version 1.1
 * @date 2024-01-05
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

#ifndef GAMESMANONE_CORE_GAME_MANAGER_H_
#define GAMESMANONE_CORE_GAME_MANAGER_H_

#include "core/types/gamesman_types.h"

/**
 * @brief Returns a NULL-terminated read-only array of all games in GAMESMAN.
 */
const Game *const *GameManagerGetAllGames(void);

/** @brief Returns the total number of games in GAMESMAN. */
int GameManagerNumGames(void);

const Game *GameManagerInitGame(ReadOnlyString game_name, void *aux);

const Game *GameManagerInitGameIndex(int index, void *aux);

const Game *GameManagerGetCurrentGame(void);

int GameManagerSetVariant(int variant_id);

void GameManagerFinalize(void);

#endif  // GAMESMANONE_CORE_GAME_MANAGER_H_

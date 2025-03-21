/**
 * @file game_list.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief List of all games.
 * @version 2.0.0
 * @date 2025-03-13
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

#ifndef GAMESMANONE_GAMES_GAME_LIST_H_
#define GAMESMANONE_GAMES_GAME_LIST_H_

#include "core/types/gamesman_types.h"

/**
 * @brief Get a NULL-ternimated array of all games.
 *
 * @return A NULL-terminated array of pointers to all implemented games
 * sorted by formal names in ascending order.
 */
const Game *const *GameListGetAllGames(void);

/**
 * @brief Returns the number of games in total.
 *
 * @return Number of games in total.
 */
int GameListGetNumGames(void);

#endif  // GAMESMANONE_GAMES_GAME_LIST_H_

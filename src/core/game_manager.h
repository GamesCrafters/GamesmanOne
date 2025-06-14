/**
 * @file game_manager.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The Game Manager Module which handles game initialization and
 * finalization.
 * @version 1.2.0
 * @date 2025-05-11
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

/**
 * @brief Initializes the game module corresponding to the given GAME_NAME.
 *
 * @param game_name Internal game name.
 * @param aux Axiliary parameter.
 * @return Read-only pointer to the game initialized on success, or
 * @return NULL otherwise.
 */
const Game *GameManagerInitGame(ReadOnlyString game_name, void *aux);

/**
 * @brief Initializes the game module corresponding to the given INDEX of the
 * game in the list of all games.
 *
 * @param index Index of the game in the list of all games.
 * @param aux Axiliary parameter.
 * @return Read-only pointer to the game initialized on success, or
 * @return NULL otherwise.
 */
const Game *GameManagerInitGameIndex(int index, void *aux);

/**
 * @brief Returns the current game initialized and loaded into the GAMESMAN
 * system, or NULL if no game has been loaded.
 */
const Game *GameManagerGetCurrentGame(void);

/**
 * @brief Returns whether the currently loaded game supports MPI.
 *
 * @return 1 if yes,
 * @return 0 otherwise.
 */
int GameManagerCurrentGameSupportsMpi(void);

/**
 * @brief Returns the total number of variants of the currently loaded game.
 *
 * @return The total number of variants of the currently loaded game.
 */
int GameManagerGetNumVariants(void);

/**
 * @brief Set the variant of the current loaded game to the variant of index
 * VARIANT_ID.
 *
 * @param variant_id Index of the variant to set to.
 * @return 0 on success, non-zero error code otherwise.
 */
int GameManagerSetVariant(int variant_id);

/** @brief Finalizes the Game Manager Module and free all allocated memory. */
void GameManagerFinalize(void);

#endif  // GAMESMANONE_CORE_GAME_MANAGER_H_

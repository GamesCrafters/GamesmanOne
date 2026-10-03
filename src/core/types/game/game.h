/**
 * @file game.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Generic Game type and related constants.
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

#ifndef GAMESMANONE_CORE_TYPES_GAME_GAME_H_
#define GAMESMANONE_CORE_TYPES_GAME_GAME_H_

#include "core/types/game/game_variant.h"
#include "core/types/gameplay_api/gameplay_api.h"
#include "core/types/solver/solver.h"
#include "core/types/uwapi/uwapi.h"

/**
 * @brief Constants used by the `Game` type.
 */
enum GameConstants {
    kGameNameLengthMax = 31,        /**< Max length of an internal game name. */
    kGameFormalNameLengthMax = 127, /**< Max length of a formal game name. */
};

/**
 * @brief Generic `Game` type.
 *
 * @details The `Game` type is an abstract type of a generic game that can be
 * solved through the GAMESMAN system. To implement a new game, correctly set
 * all member variables and function pointers that are marked as REQUIRED. A
 * game should have an internal name, a human-readable formal name for textUI
 * display, a `Solver` to use, a set of implemented API functions for the
 * chosen solver, a set of implemented API functions for the gameplay system,
 * functions to initialize and finalize the game module, and functions to
 * get/set the current game variant. The solver interface is required for
 * solving the game. The gameplay interface is required for the textUI play
 * loop and debugging. The game variant interface is optional, and may be set
 * to `NULL` if there is only one variant. Optionally, the UWAPI functions can
 * be implemented to connect the game to the web interface provided by
 * GamesCraftersUWAPI and present the game through the GamesmanUni web GUI.
 */
typedef struct Game {
    /**
     * Internal name of the game. Must contain no white spaces or special
     * characters. This member field is REQUIRED.
     */
    char name[kGameNameLengthMax + 1];

    /**
     * Human-readable name of the game. This member field is REQUIRED.
     */
    char formal_name[kGameFormalNameLengthMax + 1];

    /**
     * Solver to use. This member field is REQUIRED.
     */
    const Solver *solver;

    /**
     * Pointer to an object containing implemented API functions for the
     * selected `Solver`. This member field is REQUIRED.
     */
    const void *solver_api;

    /**
     * Pointer to a `GameplayApi` object that contains implemented gameplay API
     * functions. This member field is REQUIRED.
     */
    const GameplayApi *gameplay_api;

    /**
     * Pointer to a `Uwapi` object that contains implemented UWAPI functions.
     * This member field is optional. Implement this API to connect the game to
     * UWAPI.
     */
    const Uwapi *uwapi;

    /**
     * @brief Initializes the game module.
     *
     * @note This member function is REQUIRED.
     *
     * @param[in,out] aux Auxiliary parameter.
     *
     * @returns 0 on success, non-zero error code otherwise.
     */
    int (*Init)(void *aux);

    /**
     * @brief Finalizes the game module, freeing all allocated memory.
     *
     * @note This member function is REQUIRED.
     *
     * @returns 0 on success, non-zero error code otherwise.
     */
    int (*Finalize)(void);

    /**
     * @brief Returns the current variant of the game as a read-only
     * `GameVariant` object.
     *
     * @note This member function is optional and can be set to `NULL` if the
     * game has only one variant.
     *
     * @returns The current variant of the game.
     */
    const GameVariant *(*GetCurrentVariant)(void);

    /**
     * @brief Sets the game variant option with index `option` to the choice of
     * index `selection`.
     *
     * @note This member function is optional and can be set to `NULL` if the
     * game has only one variant.
     *
     * @param[in] option Index of the option to modify.
     * @param[in] selection Index of the choice to select.
     *
     * @returns 0 on success, non-zero error code otherwise.
     */
    int (*SetVariantOption)(int option, int selection);
} Game;

#endif  // GAMESMANONE_CORE_TYPES_GAME_GAME_H_

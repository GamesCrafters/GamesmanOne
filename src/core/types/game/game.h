/**
 * @file game.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Declaration of the Game type and related constants.
 * @details The Game type is an abstract type of a generic game that can be
 * solved through the Gamesman system. To implement a new game, correctly set
 * all members variables and function pointers that are marked as REQUIRED.
 * @version 1.0.0
 * @date 2024-01-21
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

#ifndef GAMESMANONE_CORE_TYPES_GAME_GAME_H
#define GAMESMANONE_CORE_TYPES_GAME_GAME_H

#include "core/types/game/game_variant.h"
#include "core/types/gameplay_api/gameplay_api.h"
#include "core/types/solver.h"
#include "core/types/uwapi/uwapi.h"

/** @brief Constants used by the Game type. */
enum GameConstants {
    kGameNameLengthMax = 31,        /**< Max length of an internal game name. */
    kGameFormalNameLengthMax = 127, /**< Max length of a formal game name. */
};

/**
 * @brief Generic Game type.
 *
 * @details A game should have an internal name, a human-readable formal name
 * for textUI display, a Solver to use, a set of implemented API functions for
 * the chosen Solver, a set of implemented API functions for the game play
 * system, functions to initialize and finalize the game module, and functions
 * to get/set the current game variant. The solver interface is required for
 * solving the game. The gameplay interface is required for textUI play loop
 * and debugging. The game variant interface is optional, and may be set to
 * NULL if there is only one variant.
 *
 * @par Optionally, the UWAPI functions can be implemented to connect the game
 * to the web interface provided by GamesCraftersUWAPI and present the game
 * through the GamesmanUni web GUI.
 */
typedef struct Game {
    /**
     * @brief Internal name of the game. Must contain no white spaces or
     * special characters.
     *
     * @note This member field is REQUIRED.
     */
    char name[kGameNameLengthMax + 1];

    /**
     * @brief Human-readable name of the game.
     *
     * @note This member field is REQUIRED.
     */
    char formal_name[kGameFormalNameLengthMax + 1];

    /**
     * @brief Solver to use.
     *
     * @note This member field is REQUIRED.
     */
    const Solver *solver;

    /**
     * @brief Pointer to an object containing implemented API functions for the
     * selected Solver.
     *
     * @note This member field is REQUIRED.
     */
    const void *solver_api;

    /**
     * @brief Pointer to a GameplayApi object that contains implemented gameplay
     * API functions.
     *
     * @note This member field is REQUIRED.
     */
    const GameplayApi *gameplay_api;

    /**
     * @brief Pointer to a Uwapi object that contains implemented UWAPI
     * functions.
     *
     * @note This member field is optional. Implement this API to connect the
     * game to UWAPI.
     */
    const Uwapi *uwapi;

    /**
     * @brief Initializes the game module.
     *
     * @note This member function is REQUIRED.
     *
     * @param aux Auxiliary parameter.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Init)(void *aux);

    /**
     * @brief Finalizes the game module, freeing all allocated memory.
     *
     * @note This member function is REQUIRED.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Finalize)(void);

    /**
     * @brief Returns the current variant of the game as a read-only GameVariant
     * object. Set to NULL if the game has only one variant.
     *
     * @note This member function is optional and can be set to NULL if the game
     * has only one variant.
     */
    const GameVariant *(*GetCurrentVariant)(void);

    /**
     * @brief Sets the game variant option with index OPTION to the choice of
     * index SELECTION. Set to NULL if the game has only one variant.
     *
     * @note This member function is optional and can be set to NULL if the game
     * has only one variant.
     *
     * @param option Index of the option to modify.
     * @param selection Index of the choice to select.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*SetVariantOption)(int option, int selection);
} Game;

#endif  // GAMESMANONE_CORE_TYPES_GAME_GAME_H

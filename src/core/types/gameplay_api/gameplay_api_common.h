/**
 * @file gameplay_api_common.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The GameplayApiCommon type.
 * @details A GameplayApiCommon object contains a set of constants and API
 * functions that all types of games should implement as part of their gameplay
 * API. To implement a GameplayApiCommon, the game developer should correctly
 * set all constants and implement all required functions as specified by the
 * type definition. All member variables and functions of GameplayApiCommon are
 * REQUIRED unless otherwise specified.
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

#ifndef GAMESMANONE_CORE_TYPES_GAMEPLAY_API_COMMON_H_
#define GAMESMANONE_CORE_TYPES_GAMEPLAY_API_COMMON_H_

#include "core/types/base.h"

/**
 * @brief Collection of Gameplay API functions common to all types of games.
 * 
 * @note The implementation of all functions are REQUIRED unless otherwise
 * specified.
 */
typedef struct GameplayApiCommon {
    /**
     * @brief If the game is a tier game, returns the initial position inside
     * the initial tier. Otherwise, returns the initial position. Different game
     * variants may have different initial positions.
     */
    Position (*GetInitialPosition)(void);

    /**
     * @brief Maximum length of a position string not including the
     * NULL-terminating character ('\0') as a constant integer.
     *
     * @details The gameplay system will try to allocate this amount of space
     * plus one additional byte for the NULL-terminating character ('\0') as
     * buffer for position strings. It is the game developer's responsibility to
     * precalculate this value and make sure that enough space is provided.
     */
    int position_string_length_max;

    /**
     * @brief Maximum length of a move string not including the NULL-terminating
     * character ('\0') as a constant integer.
     *
     * @details The gameplay system will try to allocate this amount of space
     * plus one additional byte for the NULL-terminating character ('\0') as
     * buffer for move strings. It is the game developer's responsibility to
     * precalculate this value and make sure that enough space is provided.
     *
     * @note Keep in mind that one must avoid letting a move string be one
     * of the following strings: "b", "q", "u", "v", as these are reserved.
     */
    int move_string_length_max;

    /**
     * @brief Converts MOVE into a move string and stores it in BUFFER.
     *
     * @note Assumes that MOVE is valid and BUFFER has enough space to hold the
     * move string. Results in undefined behavior otherwise.
     *
     * @note Keep in mind that one must avoid letting a move string be one
     * of the following strings: "b", "q", "u", "v", as these are reserved.
     */
    int (*MoveToString)(Move move, char *buffer);

    /**
     * @brief Returns true if the given MOVE_STRING is recognized as a valid
     * move string for the current game, or false otherwise.
     *
     * @param move_string User-provided move string to be validated. The user is
     * typically the user of GAMESMAN interactive through the text user
     * interface.
     */
    bool (*IsValidMoveString)(ReadOnlyString move_string);

    /**
     * @brief Converts the MOVE_STRING to a Move and returns it.
     *
     * @note Assmues that the given MOVE_STRING is valid. Results in undefined
     * behavior otherwise. Therefore, it is the developer of the gameplay
     * system's responsibility to validate the MOVE_STRING using the
     * IsValidMoveString function before calling this function.
     */
    Move (*StringToMove)(ReadOnlyString move_string);
} GameplayApiCommon;

#endif  // GAMESMANONE_CORE_TYPES_GAMEPLAY_API_COMMON_H_

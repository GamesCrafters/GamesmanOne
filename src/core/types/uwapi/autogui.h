/**
 * @file autogui.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Utilities for AutoGUI. Refer to
 * https://github.com/GamesCrafters/GamesmanUni
 * @version 1.0.0
 * @date 2024-11-14
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

#ifndef GAMESMANONE_CORE_TYPES_UWAPI_AUTOGUI_H_
#define GAMESMANONE_CORE_TYPES_UWAPI_AUTOGUI_H_

#include "core/data_structures/cstring.h"
#include "core/types/base.h"

/**
 * @brief Returns a formatted AutoGUI v2/3 position string. The caller of this
 * funciton is responsible for deallocating the \c CString returned.
 *
 * @param turn Either 1 or 2, indicating whose turn it is.
 * @param entities A NULL-terminated string, typically a board string. However,
 * the GUI designer may decide to include more entities than board slots, in
 * which case the string should be an entity string instead. Must be of length
 * less than or equal to the number of centers specified in UWAPI.
 * @return Formatted AutoGUI position string.
 */
CString AutoGuiMakePosition(int turn, ReadOnlyString entities);

/**
 * @brief Returns a formatted AutoGUI v2/3 A-type move string. The caller of
 * this funciton is responsible for deallocating the \c CString returned.
 *
 * @param token Character for the move token.
 * @param center Index of the center at which the move token will be displayed.
 * @param sound Character for the sound effect. Pass the zero-terminator '\0' to
 * omit the sound character and fall back to AutoGUI v2.
 * @return Formatted AutoGUI A-type move string.
 */
CString AutoGuiMakeMoveA(char token, int center, char sound);

/**
 * @brief Returns a formatted AutoGUI v2/3 M-type move string. The caller of
 * this funciton is responsible for deallocating the \c CString returned.
 *
 * @param src Index of the source center.
 * @param dest Index of the destination center.
 * @param sound Character for the sound effect. Pass the zero-terminator '\0' to
 * omit the sound character and fall back to AutoGUI v2.
 * @return Formatted AutoGUI M-type move string.
 */
CString AutoGuiMakeMoveM(int src, int dest, char sound);

/**
 * @brief Returns a formatted AutoGUI v2/3 L-type move string. The caller of
 * this funciton is responsible for deallocating the \c CString returned.
 *
 * @param src Index of the source center.
 * @param dest Index of the destination center.
 * @param sound Character for the sound effect. Pass the zero-terminator '\0' to
 * omit the sound character and fall back to AutoGUI v2.
 * @return Formatted AutoGUI L-type move string.
 */
CString AutoGuiMakeMoveL(int src, int dest, char sound);

/**
 * @brief Returns a formatted AutoGUI v2/3 T-type move string. The caller of
 * this funciton is responsible for deallocating the \c CString returned.
 *
 * @param text Text to be displayed as the move token.
 * @param center Index of the center at which the move token will be displayed.
 * @param sound Character for the sound effect. Pass the zero-terminator '\0' to
 * omit the sound character and fall back to AutoGUI v2.
 * @return Formatted AutoGUI T-type move string.
 */
CString AutoGuiMakeMoveT(ReadOnlyString text, int center, char sound);

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_AUTOGUI_H_

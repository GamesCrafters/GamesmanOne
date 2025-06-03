/**
 * @file partmove.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Partmove object and related function definitions.
 * @details Part-moves and full-moves are defined by the multipart move
 * interface provided by AutoGUI. This feature is designed to break down a move
 * that is logically one step but actually involves multiple steps into multiple
 * part-moves that are carried out more naturally over the GUI, resembling how
 * games are played in real life.
 * UWAPI Multipart move handler:
 * https://github.com/GamesCrafters/GamesCraftersUWAPI/blob/master/games/multipart_handler.py
 * @version 1.0.0
 * @date 2025-05-26
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
#ifndef GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_H_
#define GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_H_

#include "core/data_structures/cstring.h"

/**
 * @brief A part-move is a portion of a multipart move.
 */
typedef struct Partmove {
    CString autogui_move; /**< AutoGUI move string for this part-move. */
    CString formal_move;  /**< Formal move string for this part-move. */

    /**
     * NULL if and only if this part-move is the first part of the full move.
     * For all other parts of the full move, this field should be set to the
     * AutoGUI position string representing the intermediate board state before
     * this part-move is made.
     */
    CString from;

    /**
     * NULL if and only if this part-move is the last part of the full move. For
     * all other parts of the full move, this field should be set to the AutoGUI
     * position string representing the intermediate board state after this
     * part-move is made.
     */
    CString to;

    /**
     * The formal move string of the full move that this part-move if part of
     * ONLY when this part-move is the last part of the full move. For all other
     * part-moves, this field should be set to \c NULL .
     */
    CString full; /**< AutoGUI move string for this part-move. */
} Partmove;

/**
 * @brief Deallocates the Partmove object pointed to by \p p .
 *
 * @param p Pointer to the Partmove object to deallocate.
 */
void PartMoveDestroy(Partmove *p);

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_H_

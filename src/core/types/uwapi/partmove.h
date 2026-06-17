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
    /** AutoGUI move string for this part-move. */
    CString autogui_move;

    /** Formal move string for this part-move. */
    CString formal_move;

    /**
     * \c NULL if and only if this part-move is the first part of the full move.
     * For all other parts of the full move, this field should be set to the
     * AutoGUI position string representing the intermediate board state before
     * this part-move is made.
     */
    CString from;

    /**
     * \c NULL if and only if this part-move is the last part of the full move.
     * For all other parts of the full move, this field should be set to the
     * AutoGUI position string representing the intermediate board state after
     * this part-move is made.
     */
    CString to;

    /**
     * When this part-move is the last part of the full move, this field is set
     * to the formal move string of the full move. Otherwise, it is set to
     * \c NULL .
     */
    CString full;
} Partmove;

/**
 * @brief Deallocates the Partmove object pointed to by \p p .
 *
 * @param p Pointer to the Partmove object to deallocate.
 */
void PartMoveDestroy(Partmove *p);

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_H_

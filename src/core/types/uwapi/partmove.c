/**
 * @file partmove.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of Partmove related functions.
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
#include "core/types/uwapi/partmove.h"

#include "core/data_structures/cstring.h"

void PartMoveDestroy(Partmove *pm) {
    CStringDestroy(&pm->autogui_move);
    CStringDestroy(&pm->formal_move);
    CStringDestroy(&pm->from);
    CStringDestroy(&pm->to);
    CStringDestroy(&pm->full);
}

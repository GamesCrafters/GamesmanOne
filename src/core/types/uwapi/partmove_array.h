/**
 * @file partmove_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic array data structure for Partmoves.
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
#ifndef GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_ARRAY_H_
#define GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_ARRAY_H_

#include <stdint.h>  // int64_t

#include "core/data_structures/cstring.h"
#include "core/types/uwapi/partmove.h"

/**
 * @brief Dynamic array of Partmoves.
 */
typedef struct PartmoveArray {
    Partmove *array;  /**< Data. */
    int64_t size;     /**< Number of elements currently stored. */
    int64_t capacity; /**< Current capacity of the array. */
} PartmoveArray;

/**
 * @brief Initializes \p pa to an empty array.
 *
 * @param pa Pointer to an uninitialized PartmoveArray.
 */
void PartmoveArrayInit(PartmoveArray *pa);

/**
 * @brief Deallocates PartmoveArray \p pa .
 *
 * @param pa Array to deallocate.
 */
void PartmoveArrayDestroy(PartmoveArray *pa);

/**
 * @brief Creates a new Partmove object and appends it to the back of \p pa ,
 * transferring ownership of all objects of type CString using CStringInitMove.
 * @note The ownership of \p autogui_move , \p formal_move , \p from , \p to ,
 * and \p full will be transferred to \p pa after a successful call to this
 * function, leaving those CStrings in uninitialized states. The caller should
 * not deallocate or reuse those CStrings until they are reinitialized.
 *
 * @param pa Destination part-move array.
 * @param autogui_move AutoGUI move string for this part-move; non-NULL.
 * @param formal_move Formal move string for this part-move; non-NULL.
 * @param from NULL if and only if this part-move is the first part of the full
 * move. For all other parts of the full move, this field should be set to the
 * AutoGUI position string representing the intermediate board state before this
 * part-move is made.
 * @param to NULL if and only if this part-move is the last part of the full
 * move. For all other parts of the full move, this field should be set to the
 * AutoGUI position string representing the intermediate board state after this
 * part-move is made.
 * @param full The formal move string of the full move that this part-move is
 * part of ONLY when this part-move is the last part of the full move. For all
 * other part-moves, this field should be set to \c NULL .
 * @return \c kNoError on success, or
 * @return \c kMallocFailureError on memory allocation failure.
 */
int PartmoveArrayEmplaceBack(PartmoveArray *pa, CString *autogui_move,
                             CString *formal_move, CString *from, CString *to,
                             CString *full);

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_ARRAY_H

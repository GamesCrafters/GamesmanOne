/**
 * @file partmove_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic array data structure for `Partmove`s.
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

#include <stdint.h>

#include "core/data_structures/cstring.h"
#include "core/types/uwapi/partmove.h"

/**
 * @brief Dynamic array of `Partmove`s.
 */
typedef struct PartmoveArray {
    Partmove *array;  /**< Data. */
    int64_t size;     /**< Number of elements currently stored. */
    int64_t capacity; /**< Current capacity of the array. */
} PartmoveArray;

/**
 * @brief Initializes `pa` to an empty array.
 *
 * @param[out] pa Pointer to an uninitialized `PartmoveArray`.
 */
void PartmoveArrayInit(PartmoveArray *pa);

/**
 * @brief Deallocates `PartmoveArray` `pa`.
 *
 * @param[in,out] pa Array to deallocate.
 */
void PartmoveArrayDestroy(PartmoveArray *pa);

/**
 * @brief Creates a new `Partmove` object and appends it to the back of `pa`,
 * transferring ownership of all objects of type `CString` using
 * `CStringInitMove`.
 *
 * @note The ownership of `autogui_move`, `formal_move`, `from`, `to`,
 * and `full` will be transferred to `pa` after a successful call to this
 * function, leaving those `CString`s in uninitialized states. The caller should
 * not deallocate or reuse those `CString`s until they are reinitialized.
 *
 * @param[in,out] pa Destination part-move array.
 * @param[in,out] autogui_move AutoGUI move string for this part-move; non-NULL.
 * @param[in,out] formal_move Formal move string for this part-move; non-NULL.
 * @param[in,out] from `NULL` if and only if this part-move is the first part of
 * the full move. For all other parts of the full move, this parameter should be
 * set to the AutoGUI position string representing the intermediate board state
 * before this part-move is made.
 * @param[in,out] to `NULL` if and only if this part-move is the last part of
 * the full move. For all other parts of the full move, this parameter should be
 * set to the AutoGUI position string representing the intermediate board state
 * after this part-move is made.
 * @param[in,out] full When this part-move is the last part of the full move,
 * this parameter should be set to the formal move string of the full move.
 * Otherwise, it should be set to `NULL`.
 *
 * @retval kSuccess on success.
 * @retval kMallocFailureError on memory allocation failure.
 */
int PartmoveArrayEmplaceBack(PartmoveArray *pa, CString *autogui_move,
                             CString *formal_move, CString *from, CString *to,
                             CString *full);

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_ARRAY_H_

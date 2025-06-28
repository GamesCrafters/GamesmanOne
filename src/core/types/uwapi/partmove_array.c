/**
 * @file partmove_array.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of PartmoveArray.
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
#include "core/types/uwapi/partmove_array.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdint.h>  // int64_t
#include <stdlib.h>  // realloc, free
#include <string.h>  // memset

#include "core/data_structures/cstring.h"
#include "core/types/gamesman_error.h"
#include "core/types/uwapi/partmove.h"

void PartmoveArrayInit(PartmoveArray *pa) { memset(pa, 0, sizeof(*pa)); }

void PartmoveArrayDestroy(PartmoveArray *pa) {
    for (int64_t i = 0; i < pa->size; ++i) {
        PartMoveDestroy(&pa->array[i]);
    }
    free(pa->array);
    memset(pa, 0, sizeof(*pa));
}

static int PartmoveArrayExpand(PartmoveArray *pa) {
    int64_t new_capacity = (pa->capacity == 0) ? 1 : pa->capacity * 2;
    Partmove *new_array =
        (Partmove *)realloc(pa->array, new_capacity * sizeof(Partmove));
    if (new_array == NULL) return 1;

    pa->array = new_array;
    pa->capacity = new_capacity;

    return 0;
}

int PartmoveArrayEmplaceBack(PartmoveArray *pa, CString *autogui_move,
                             CString *formal_move, CString *from, CString *to,
                             CString *full) {
    // Make sure there is enough space for the new entry in PA.
    if (pa->size == pa->capacity) {
        int error = PartmoveArrayExpand(pa);
        if (error) return kMallocFailureError;
    }
    assert(pa->size < pa->capacity);

    CStringInitMove(&pa->array[pa->size].autogui_move, autogui_move);
    CStringInitMove(&pa->array[pa->size].formal_move, formal_move);
    CStringInitMove(&pa->array[pa->size].from, from);
    CStringInitMove(&pa->array[pa->size].to, to);
    CStringInitMove(&pa->array[pa->size].full, full);
    ++pa->size;

    return kNoError;
}

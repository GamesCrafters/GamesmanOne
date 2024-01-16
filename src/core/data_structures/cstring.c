/**
 * @file cstring.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic C-string (char array) implementation.
 * @version 1.0.0
 * @date 2024-01-15
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

#include "core/data_structures/cstring.h"

#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdlib.h>   // malloc, free
#include <string.h>   // memset, strlen

bool CStringInit(CString *cstring, const char *src) {
    cstring->length = -1;
    cstring->capacity = -1;
    
    int64_t length = strlen(src);
    cstring->str = (char *)malloc(length + 1);
    if (cstring->str == NULL) return false;

    strcpy(cstring->str, src);
    cstring->length = length;
    cstring->capacity = length + 1;
    return true;
}

void CStringDestroy(CString *cstring) {
    free(cstring->str);
    memset(cstring, 0, sizeof(*cstring));
}

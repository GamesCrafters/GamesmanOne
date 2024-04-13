/**
 * @file cstring.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic C-string (char array) implementation.
 * @version 2.0.0
 * @date 2024-04-07
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

bool CStringInit(CString *cstring) {
    cstring->str = (char *)calloc(1, sizeof(char));
    if (cstring->str == NULL) return false;

    cstring->length = 0;
    cstring->capacity = 1;

    return true;
}

bool CStringInitCopy(CString *cstring, const char *src) {
    if (src == NULL) return false;

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

static bool CStringExpand(CString *cstring, int64_t target_size) {
    int64_t new_capacity = cstring->capacity * 2;
    while (new_capacity <= target_size) {
        new_capacity *= 2;
    }
    char *new_str = (char *)realloc(cstring->str, new_capacity);
    if (new_str == NULL) return false;

    cstring->str = new_str;
    cstring->capacity = new_capacity;

    return true;
}

bool CStringAppend(CString *dest, const char *src) {
    int64_t append_length = strlen(src);
    int64_t target_size = dest->length + append_length;

    // Expand if necessary.
    if (target_size >= dest->capacity) {
        CStringExpand(dest, target_size);
    }

    strcat(dest->str, src);

    return true;
}

bool CStringResize(CString *cstring, int64_t size, char fill) {
    if (cstring->length >= size) {
        cstring->length = size;
        cstring->str[size] = '\0';
        return true;
    }

    // cstring->length < size, expanding.
    if (size >= cstring->capacity) {
        CStringExpand(cstring, size);
    }
    memset(cstring->str + cstring->length, fill, size - cstring->length);
    cstring->length = size;

    return true;
}

/**
 * @file cstring.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic C-string (char array) implementation.
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "core/gamesman_memory.h"

CString CStringGetNull(void) {
    return (CString){
        .str = NULL,
        .length = 0,
        .capacity = 0,
    };
}

bool CStringInitEmpty(CString *cstring) {
    if (!cstring) {
        return false;
    }

    static const size_t kInitialSize = GM_CACHE_LINE_SIZE;
    cstring->str = (char *)GamesmanCallocWhole(kInitialSize, sizeof(char));
    if (!cstring->str) {
        return false;
    }

    cstring->length = 0;
    cstring->capacity = kInitialSize;

    return true;
}

bool CStringInitCopy(CString *init, const CString *other) {
    if (!init) {
        return false;
    }

    if (!other || CStringIsNull(other)) {
        *init = CStringGetNull();
        return true;
    }

    init->str = (char *)GamesmanMalloc(other->capacity);
    if (!init->str) {
        return false;
    }

    memcpy(init->str, other->str, other->length + 1);
    init->length = other->length;
    init->capacity = other->capacity;

    return true;
}

bool CStringInitCopyCharArray(CString *cstring, const char *src) {
    if (!cstring) {
        return false;
    }

    if (!src) {
        *cstring = CStringGetNull();
        return true;
    }

    const size_t length = strlen(src);
    const size_t capacity = length + 1;
    cstring->str = (char *)GamesmanMalloc(capacity);
    if (!cstring->str) {
        return false;
    }

    memcpy(cstring->str, src, capacity);
    cstring->length = (int64_t)length;
    cstring->capacity = (int64_t)capacity;

    return true;
}

void CStringInitMove(CString *init, CString *other) {
    if (!init || init == other) {
        return;
    }

    if (!other || CStringIsNull(other)) {
        *init = CStringGetNull();
        return;
    }

    *init = *other;
    memset(other, 0, sizeof(*other));
}

void CStringDestroy(CString *cstring) {
    if (!cstring) {
        return;
    }

    GamesmanFree(cstring->str);
    memset(cstring, 0, sizeof(*cstring));
}

static bool CStringExpand(CString *cstring, int64_t target_size) {
    int64_t new_capacity = cstring->capacity * 2;
    while (new_capacity <= target_size) {
        new_capacity *= 2;
    }

    char *new_str = (char *)GamesmanMalloc(new_capacity);
    if (new_str == NULL) {
        return false;
    }

    // Copy over the content
    memcpy(new_str, cstring->str, cstring->length + 1);
    GamesmanFree(cstring->str);

    cstring->str = new_str;
    cstring->capacity = new_capacity;

    return true;
}

bool CStringAppend(CString *dest, const char *src) {
    if (!dest || !src) {
        return false;
    }

    const int64_t append_length = (int64_t)strlen(src);
    const int64_t target_size = dest->length + append_length;

    // Detect if src points inside dest->str (self-append)
    bool is_self_overlap =
        (src >= dest->str) && (src < dest->str + dest->capacity);
    ptrdiff_t src_offset = 0;
    if (is_self_overlap) {
        src_offset = src - dest->str;  // Save the relative offset
    }

    // Expand if necessary
    if (target_size >= dest->capacity) {
        if (!CStringExpand(dest, target_size)) {
            return false;
        }

        // Expansion frees the original src address. Set src to the new address
        // in the reallocated string to avoid use-after-free.
        if (is_self_overlap) {
            src = dest->str + src_offset;
        }
    }

    memcpy(dest->str + dest->length, src, append_length);
    dest->length += append_length;
    dest->str[dest->length] = '\0';

    return true;
}

bool CStringResize(CString *cstring, int64_t length, char fill) {
    if (!cstring || length < 0) {
        return false;
    }

    // Expand if current capacity is not enough for the given length.
    if (length >= cstring->capacity) {
        if (!CStringExpand(cstring, length)) {
            return false;
        }
    }

    if (cstring->length < length) {
        memset(cstring->str + cstring->length, fill, length - cstring->length);
    }

    cstring->length = length;
    cstring->str[length] = '\0';
    return true;
}

bool CStringIsNull(const CString *cstring) {
    if (!cstring) {
        return true;
    }

    return !cstring->str && !cstring->length && !cstring->capacity;
}

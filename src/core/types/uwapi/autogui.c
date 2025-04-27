/**
 * @file autogui.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Utilities for AutoGUI implementation.
 * @version 1.0.2
 * @date 2025-04-26
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

#include "core/types/uwapi/autogui.h"

#include <stddef.h>  // size_t
#include <stdio.h>   // sprintf
#include <stdlib.h>  // free
#include <string.h>  // strlen

#include "core/constants.h"
#include "core/data_structures/cstring.h"
#include "core/gamesman_memory.h"
#include "core/types/base.h"

CString AutoGuiMakePosition(int turn, ReadOnlyString entities) {
    CString ret;
    CStringInitEmpty(&ret);
    char prefix[] = "1_";  // Placeholder
    prefix[0] = (char)(turn + '0');
    CStringAppend(&ret, prefix);
    CStringAppend(&ret, entities);

    return ret;
}

CString AutoGuiMakeMoveA(char token, int center, char sound) {
    CString ret;
    char buffer[kInt64Base10StringLengthMax + 7];
    if (sound) {
        sprintf(buffer, "A_%c_%d_%c", token, center, sound);
    } else {
        sprintf(buffer, "A_%c_%d", token, center);
    }
    CStringInitCopyCharArray(&ret, buffer);

    return ret;
}

CString AutoGuiMakeMoveM(int src, int dest, char sound) {
    CString ret;
    char buffer[kInt64Base10StringLengthMax * 2 + 6];
    if (sound) {
        sprintf(buffer, "M_%d_%d_%c", src, dest, sound);
    } else {
        sprintf(buffer, "M_%d_%d", src, dest);
    }
    CStringInitCopyCharArray(&ret, buffer);

    return ret;
}

CString AutoGuiMakeMoveL(int src, int dest, char sound) {
    CString ret;
    char buffer[kInt64Base10StringLengthMax * 2 + 6];
    if (sound) {
        sprintf(buffer, "L_%d_%d_%c", src, dest, sound);
    } else {
        sprintf(buffer, "L_%d_%d", src, dest);
    }
    CStringInitCopyCharArray(&ret, buffer);

    return ret;
}

CString AutoGuiMakeMoveT(ReadOnlyString text, int center, char sound) {
    CString ret;
    size_t len = strlen(text);
    char *buffer =
        (char *)SafeCalloc(len + kInt32Base10StringLengthMax + 6, sizeof(char));
    if (sound) {
        sprintf(buffer, "T_%s_%d_%c", text, center, sound);
    } else {
        sprintf(buffer, "T_%s_%d", text, center);
    }
    CStringInitCopyCharArray(&ret, buffer);
    GamesmanFree(buffer);

    return ret;
}

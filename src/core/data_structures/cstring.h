/**
 * @file cstring.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic C-string (char array).
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_CSTRING_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_CSTRING_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

/** @brief Dynamic C-string. */
typedef struct CString {
    char *str;        /**< Dynamic string. */
    int64_t length;   /**< Length of the string. */
    int64_t capacity; /**< Capacity of the dynamic space. */
} CString;

/**
 * @brief Initializes CSTRING to the same string as SRC.
 *
 * @details On success, CSTRING will have the same length as SRC with capacity
 * equal to its length.
 *
 * @param cstring Target CString, which is assumed to be uninitialized.
 * @param src Source string.
 * @return true on success,
 * @return false otherwise.
 */
bool CStringInit(CString *cstring, const char *src);

/** @brief Destroys the given CSTRING. */
void CStringDestroy(CString *cstring);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_CSTRING_H_

/**
 * @file cstring.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic C-string (char array).
 * @version 3.0.1
 * @date 2024-12-20
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
 * @brief This \c CString object is used as a "NULL" value.
 */
extern const CString kNullCString;

/**
 * @brief This \c CString object is used to indicate an error.
 */
extern const CString kErrorCString;

/**
 * @brief Initializes an empty CSTRING, which contains only the NULL character
 * ('\0').
 *
 * @param cstring Target CString, which is assumed to be uninitialized.
 *
 * @return true on success,
 * @return false otherwise.
 */
bool CStringInitEmpty(CString *cstring);

/**
 * @brief Initializes CSTRING to the same string as OTHER if OTHER is non-NULL;
 * initializes CSTRING to kNullCString otherwise.
 *
 * @details On success, CSTRING will have the same length as SRC with capacity
 * equal to its length if SRC is non-NULL.
 *
 * @param init Target CString, which is assumed to be uninitialized.
 * @param src Source CString.
 * @return true on success,
 * @return false otherwise.
 */
bool CStringInitCopy(CString *init, const CString *other);

/**
 * @brief Initializes CSTRING to the same string as SRC if SRC is non-NULL;
 * initializes CSTRING to kNullCString otherwise.
 *
 * @details On success, CSTRING will have the same length as SRC with capacity
 * equal to its length if SRC is non-NULL.
 *
 * @param cstring Target CString, which is assumed to be uninitialized.
 * @param src Source string.
 * @return true on success,
 * @return false otherwise.
 */
bool CStringInitCopyCharArray(CString *cstring, const char *src);

/**
 * @brief Initializes \p init by taking ownership of \p other if \p other is
 * non-NULL. In this case, \p other is left in an undefined state after the call
 * to this function. If \p other is \c NULL, then \p init is initialized to
 * \c kNullCString.
 *
 * @param init \c CString to initialize.
 * @param other \c CString to move.
 */
void CStringInitMove(CString *init, CString *other);

/** @brief Destroys the given CSTRING. */
void CStringDestroy(CString *cstring);

/**
 * @brief Appends SRC to the end of DEST CString.
 *
 * @param dest Destination CString.
 * @param src Source string.
 * @return true on success,
 * @return false otherwise.
 */
bool CStringAppend(CString *dest, const char *src);

/**
 * @brief Resizes CSTRING to SIZE. If SIZE is greater than the current size of
 * CSTRING, characters beyond SIZE are removed; if SIZE is smaller than the
 * current size of CSTRING, the current content is extended to SIZE characters
 * by inserting at the end as many FILL characters as needed to reach the
 * specified SIZE.
 *
 * @param cstring CString to resize.
 * @param size New size of the CString.
 * @param fill Fill in the new space with this character. Ignored if SIZE is
 * smaller than the current size of CSTRING.
 * @return true on success,
 * @return false otherwise.
 */
bool CStringResize(CString *cstring, int64_t size, char fill);

/**
 * @brief Returns \c true if \p cstring is \c kNullCString, or false otherwise.
 */
bool CStringIsNull(const CString *cstring);

/**
 * @brief Returns \c true if \p cstring is \c kErrorCString, or false otherwise.
 */
bool CStringError(const CString *cstring);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_CSTRING_H_

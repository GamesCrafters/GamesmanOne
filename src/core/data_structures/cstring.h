/**
 * @file cstring.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic C-string (char array) definition and operations.
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

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Dynamic C-string.
 */
typedef struct CString {
    char *str;        /**< Null-terminated string content. */
    int64_t length;   /**< Length of the string. */
    int64_t capacity; /**< Capacity of the dynamic space. */
} CString;

/**
 * @brief Returns a null `CString` object.
 *
 * @returns A `CString` struct initialized to the null state.
 */
CString CStringGetNull(void);

/**
 * @brief Initializes an empty `CString`, which contains only the NULL character
 * (`\0`).
 *
 * @param[out] cstring Target `CString`, which is assumed to be uninitialized.
 *
 * @retval true On success.
 * @retval false Otherwise.
 */
bool CStringInitEmpty(CString *cstring);

/**
 * @brief Initializes `init` to the same string as `other` if `other` is
 * non-NULL and not the null string; initializes `init` to the null string
 * otherwise.
 *
 * @details On success, `init` will have the same length as `other` with
 * capacity equal to its length.
 *
 * @param[out] init Target `CString`, which is assumed to be uninitialized.
 * @param[in] other Source `CString`.
 *
 * @retval true On success.
 * @retval false Otherwise.
 */
bool CStringInitCopy(CString *init, const CString *other);

/**
 * @brief Initializes `cstring` to the same string as `src` if `src` is
 * non-NULL; initializes `cstring` to the null string otherwise.
 *
 * @details On success, `cstring` will have the same length as `src` with
 * capacity equal to its length.
 *
 * @param[out] cstring Target `CString`, which is assumed to be uninitialized.
 * @param[in] src Source null-terminated char array.
 *
 * @retval true On success.
 * @retval false Otherwise.
 */
bool CStringInitCopyCharArray(CString *cstring, const char *src);

/**
 * @brief Initializes `init` by taking ownership of `other` if `other` is
 * non-NULL and not the null string.
 *
 * @details In this case, `other` is left in the null string state after the
 * call to this function. If `other` is `NULL` or the null string, then `init`
 * is initialized to the null string. If `init` is `NULL`, then the function
 * call is a no-op.
 *
 * @param[out] init `CString` to initialize.
 * @param[in,out] other `CString` to move.
 */
void CStringInitMove(CString *init, CString *other);

/**
 * @brief Destroys the given `CString` and frees its resources.
 *
 * @param[in,out] cstring `CString` to destroy.
 */
void CStringDestroy(CString *cstring);

/**
 * @brief Appends `src` to the end of destination `dest`.
 *
 * @param[in,out] dest Destination `CString`.
 * @param[in] src Source string.
 *
 * @retval true On success.
 * @retval false Otherwise.
 */
bool CStringAppend(CString *dest, const char *src);

/**
 * @brief Resizes `cstring` to `length`.
 *
 * @details If `length` is less than the current length of `cstring`,
 * characters beyond `length` are removed; if `length` is greater than the
 * current length of `cstring`, the current content is extended to `length`
 * characters by inserting at the end as many `fill` characters as needed to
 * reach the specified `length`.
 *
 * @param[in,out] cstring `CString` to resize.
 * @param[in] length New length of the `CString`.
 * @param[in] fill Fill in the new space with this character. Ignored if
 * `length` is less than the current length of `cstring`.
 *
 * @retval true On success.
 * @retval false Otherwise.
 */
bool CStringResize(CString *cstring, int64_t length, char fill);

/**
 * @brief Returns if `cstring` is the null string.
 *
 * @param[in] cstring The `CString` to check.
 *
 * @retval true If `cstring` is NULL or represents the null string state.
 * @retval false Otherwise.
 */
bool CStringIsNull(const CString *cstring);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_CSTRING_H_

/**
 * @file xstring.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @brief Library of file operation wrapper functions.
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

#ifndef GAMESMANONE_LIBS_STRING_XSTRING_H_
#define GAMESMANONE_LIBS_STRING_XSTRING_H_

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Safely appends formatted text to a buffer, tracking the write offset.
 *
 * @details This function relies on
 * [vsnprintf](https://en.cppreference.com/w/c/io/vfprintf) to append formatted
 * string data to `buffer` starting at `*offset`. It strictly bounds the write
 * up to `max_len`. If truncation occurs, `*offset` is set to `max_len - 1`. If
 * an encoding error occurs, `*offset` is set to `SIZE_MAX`. If `*offset` equals
 * `SIZE_MAX` or indicates the buffer is full, the function returns immediately
 * without modifying the buffer. The function will also be a no-op if either
 * `buffer` or `offset` is `NULL`.
 *
 * @note Using this function repeatedly is safe even if a previous call failed
 * or truncated, as the `offset` state is checked before any operation.
 *
 * @param[out] buffer The destination string buffer where characters are
 * appended.
 * @param[in] max_len The total maximum capacity of the destination buffer.
 * @param[in,out] offset Pointer to the current offset index in the buffer.
 * @param[in] format The format string, following standard `printf`
 * specifications.
 * @param[in] ... Additional variadic arguments required by the format string.
 */
void AppendSnprintf(char *__restrict buffer, size_t max_len, size_t *offset,
                    const char *__restrict format, ...);

/**
 * @brief Checks if a string matches a given regular expression.
 *
 * @param[in] pattern The regular expression pattern.
 * @param[in] target  The string to test against the pattern.
 *
 * @retval true if it matches,
 * @retval false otherwise.
 */
bool RegexMatch(const char *pattern, const char *target);

#endif  // GAMESMANONE_LIBS_STRING_XSTRING_H_

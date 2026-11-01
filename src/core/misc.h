/**
 * @file misc.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Miscellaneous utility functions.
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

#ifndef GAMESMANONE_CORE_MISC_H_
#define GAMESMANONE_CORE_MISC_H_

#include <stddef.h>

/**
 * @brief Gracefully exits GAMESMAN.
 *
 * @warning This function calls `exit()` which is not MT-safe. Do not call this
 * function in a multithreaded code section.
 */
void GamesmanExit(void);

/**
 * @brief Prints a fatal error message and terminates GAMESMAN.
 *
 * @param[in] message A message to include as part of the fatal error message.
 * If `NULL`, a generic message will be printed.
 */
void NotReached(const char *message);

/**
 * @brief Formats a time given in seconds into a human-readable string.
 *
 * Formats the time into the format "[YYYY y MM m DD d HH h MM m ]SS s",
 * appending higher-order time units only if they are strictly positive.
 * If `seconds` is negative, writes "NEGATIVE TIME ERROR" to `buf`.
 * If `seconds` exceeds 9999 years or `INT64_MAX`, writes "INFINITE" to `buf`.
 *
 * @param[in] seconds The time in seconds to format.
 * @param[out] buf The buffer to write the formatted string into.
 * @param[in] buf_size The size of `buf` in bytes. Should be at least 32 to
 * accommodate all possible outputs (including the NULL-terminator).
 *
 * @return A pointer to `buf`, or `NULL` if `buf` is `NULL` or `buf_size` is 0.
 */
char *SecondsToFormattedTimeString(double seconds, char *buf, size_t buf_size);

#endif  // GAMESMANONE_CORE_MISC_H_

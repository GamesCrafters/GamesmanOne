/**
 * @file misc.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Miscellaneous utility functions.
 * @version 2.0.0
 * @date 2025-03-18
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
 * @warning This function calls exit() which is not MT-safe. Do not call this
 * function in a multithreaded code section.
 */
void GamesmanExit(void);

/** @brief Prints the error MESSAGE and terminates GAMESMAN. */
void NotReached(const char *message);

/**
 * @brief Returns the time equivalent to SECONDS seconds in the format of "[YYYY
 * y MM m DD d HH h MM m ]SS s" as a c-string. buf_size should be at least 32 to
 * accomodate for all possible outputs (NULL-terminator included).
 */
char *SecondsToFormattedTimeString(double seconds, char *buf, size_t buf_size);

#endif  // GAMESMANONE_CORE_MISC_H_

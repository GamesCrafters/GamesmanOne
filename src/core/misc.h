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

#include "core/types/base.h"

/**
 * @brief Gracefully exits GAMESMAN.
 * @warning This function calls exit() which is not MT-safe. Do not call this
 * function in a multithreaded code section.
 */
void GamesmanExit(void);

/** @brief Prints the error MESSAGE and terminates GAMESMAN. */
void NotReached(ReadOnlyString message);

/**
 * @brief Equivalent to first calling printf with the given parameters and then
 * calling fflush(stdout).
 */
void PrintfAndFlush(const char *format, ...);

/**
 * @brief Prints \p prompt followed by a new line ('\n') and an arrow ("=>") to
 * \c stdout, and then reads in X characters from \c stdin until a new line or
 * EOF is encountered but only writes up to \p length_max characters to \p buf,
 * not including the trailing new line character ('\n').
 *
 * @note \p buf is assumed to have enough space to hold at least \p length_max +
 * 1 characters to include the terminal '\0'.
 *
 * @param prompt A prompt to be printed out that explains what the user input
 * should be.
 * @param buf Output parameter. The user input, up to \p length_max bytes, is
 * stored in the contiguous space that this pointer is pointing to.
 * @param length_max Maximum acceptable user input length in number of
 * characters.
 * @return \p buf.
 */
char *PromptForInput(ReadOnlyString prompt, char *buf, int length_max);

/**
 * @brief Returns the time equivalent to SECONDS seconds in the format of "[YYYY
 * y MM m DD d HH h MM m ]SS s" as a c-string.
 */
char *SecondsToFormattedTimeString(double seconds);

#endif  // GAMESMANONE_CORE_MISC_H_

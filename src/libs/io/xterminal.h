/**
 * @file xterminal.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Library of terminal I/O helper functions.
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

#ifndef GAMESMANONE_LIBS_IO_XTERMINAL_H_
#define GAMESMANONE_LIBS_IO_XTERMINAL_H_

/**
 * @brief Equivalent to first calling `printf` with the given parameters and
 * then calling `fflush(stdout)`.
 *
 * @param[in] format The format string.
 * @param[in] ... Variadic arguments matching the format string.
 */
void PrintfAndFlush(const char *format, ...);

/**
 * @brief Prints `prompt` followed by a new line (`\n`) and an arrow (`=> `) to
 * `stdout`, and then reads characters from `stdin` until a new line or EOF is
 * encountered. It writes up to `max_length` characters to `buf`, discards any
 * excess input, and strips trailing carriage returns and new lines.
 *
 * @note `buf` is assumed to have enough space to hold at least `max_length` + 1
 * characters to include the terminal `\0`.
 *
 * @param[in] prompt A prompt to be printed out that explains what the user
 * input should be.
 * @param[out] buf Output parameter. The user input, up to `max_length` bytes,
 * is stored in the contiguous space that this pointer is pointing to.
 * @param[in] max_length Maximum acceptable user input length in number of
 * characters.
 *
 * @returns The populated buffer `buf`.
 * @retval NULL If `fgets` encounters an error or reaches EOF without reading
 * any characters.
 */
char *PromptForInput(const char *prompt, char *buf, int max_length);

#endif  // GAMESMANONE_LIBS_IO_XTERMINAL_H_

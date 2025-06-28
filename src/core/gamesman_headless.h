/**
 * @file gamesman_headless.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief GAMESMAN headless mode.
 * @version 1.3.0
 * @date 2025-05-11
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

#ifndef GAMESMANONE_CORE_GAMESMAN_HEADLESS_H_
#define GAMESMANONE_CORE_GAMESMAN_HEADLESS_H_

/**
 * @brief The main entry point of the headless mode of GAMESMAN.
 *
 * @param argc Number of arguments passed in from the command line. Copied over
 * from the main function.
 * @param argv Array of arguments passed in from the command line. Copied over
 * from the main function.
 * @return 0 on successful exit, or
 * @return non-zero error code otherwise.
 */
int GamesmanHeadlessMain(int argc, char **argv);

#endif  // GAMESMANONE_CORE_GAMESMAN_HEADLESS_H_

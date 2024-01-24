/**
 * @file hutils.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Common utility functions for headless mode.
 * @version 1.0.0
 * @date 2024-01-20
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

#ifndef GAMESMANONE_CORE_HEADLESS_HUTILS_H_
#define GAMESMANONE_CORE_HEADLESS_HUTILS_H_

#include <stdbool.h>  // bool

#include "core/types/gamesman_types.h"

/**
 * @brief Returns the level of verbosity to use given the two user-specified
 * options VERBOSE and QUEIT.
 *
 * @return 0 if VERBOSE is false and QUEIT is true,
 * @return 1 if both VERBOSE and QUEIT are false, or
 * @return 2 if VERBOSE is true (the value of QUIET will be ignored.)
 */
int HeadlessGetVerbosity(bool verbose, bool quiet);

/**
 * @brief Redirects stdout to the given OUTPUT file path using freopen, or does
 * nothing if OUTPUT is NULL.
 *
 * @param output Output file path. NULL to use default stdout.
 * @return 0 on success, non-zero error code otherwise.
 */
int HeadlessRedirectOutput(ReadOnlyString output);

/**
 * @brief Initializes the solver for game of name GAME_NAME and variant index
 * VARIANT_ID.
 *
 * @param game_name Name of the game used internally by GAMESMAN.
 * @param variant_id Index of the variant to initialize. If negative, the
 * default variant will be initialized.
 * @param data_path Path to the "data" directory. The default path will be used
 * if set to NULL.
 * @return 0 on success, non-zero error code otherwise.
 */
int HeadlessInitSolver(ReadOnlyString game_name, int variant_id,
                       ReadOnlyString data_path);

#endif  // GAMESMANONE_CORE_HEADLESS_HUTILS_H_

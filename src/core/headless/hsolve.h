/**
 * @file hsolve.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Game solving functionality of headless mode.
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

#ifndef GAMESMANONE_CORE_HEADLESS_HSOLVE_H_
#define GAMESMANONE_CORE_HEADLESS_HSOLVE_H_

#include <stdbool.h>  // bool

#include "core/types/gamesman_types.h"

/**
 * @brief Solves the game of name GAME_NAME and variant index VARIANT_ID and
 * stores the database at the given DATA_PATH.
 *
 * @param game_name Name of the game used internally by Gamesman.
 * @param variant_id Index of the variant to solve for. If negative, the default
 * variant will be solved.
 * @param data_path Path to the "data" directory. The default path will be used
 * if set to NULL.
 * @param force If set to true, the given game variant will be solved regardless
 * of the current database status. Otherwise, the solving process is skipped if
 * the game variant has already been correctly solved.
 * @param verbose May take values 0, 1, or 2. If set to 0, no output will be
 * produced unless an error occurrs. If set to 1, the solver will print out the
 * default messages. If set to 2, additional information will be printed.
 * @return 0 on success, non-zero error code otherwise.
 */
int HeadlessSolve(ReadOnlyString game_name, int variant_id,
                  ReadOnlyString data_path, bool force, int verbose);

#endif  // GAMESMANONE_CORE_HEADLESS_HSOLVE_H_

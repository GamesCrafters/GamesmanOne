/**
 * @file hquery.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Position querying functionality of headless mode.
 * @version 1.1.0
 * @date 2024-10-21
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

#ifndef GAMESMANONE_CORE_HEADLESS_HQUERY_H_
#define GAMESMANONE_CORE_HEADLESS_HQUERY_H_

#include "core/types/gamesman_types.h"

/**
 * @brief Prints out a detailed position response for the given FORMAL_POSITION
 * of game GAME_NAME, variant index VARIANT_ID in the UWAPI json format to
 * stdout.
 *
 * @param game_name Name of the game used internally by GAMESMAN.
 * @param variant_id Index of the variant to load. If negative, the default
 * variant will be loaded.
 * @param data_path Path to the "data" directory. The default path will be used
 * if set to NULL.
 * @param formal_position Formal position string to query.
 * @return 0 on success, non-zero error code otherwise.
 */
int HeadlessQuery(ReadOnlyString game_name, int variant_id,
                  ReadOnlyString data_path, ReadOnlyString formal_position);

/**
 * @brief Prints out a start position response for game of name GAME_NAME and
 * variant index VARIANT_ID.
 *
 * @param game_name Name of the game used internally by GAMESMAN.
 * @param variant_id Index of the variant to load. If negative, the default
 * variant will be loaded.
 * @return 0 on success, non-zero error code otherwise.
 */
int HeadlessGetStart(ReadOnlyString game_name, int variant_id);

/**
 * @brief Prints out a random position response for game of name GAME_NAME and
 * variant index VARIANT_ID, if implemented. Prints out an error response and
 * returns kNotImplementedError otherwise.
 *
 * @param game_name Name of the game used internally by GAMESMAN.
 * @param variant_id Index of the variant to load. If negative, the default
 * variant will be loaded.
 * @return 0 on success, non-zero error code otherwise.
 */
int HeadlessGetRandom(ReadOnlyString game_name, int variant_id);

#endif  // GAMESMANONE_CORE_HEADLESS_HQUERY_H_

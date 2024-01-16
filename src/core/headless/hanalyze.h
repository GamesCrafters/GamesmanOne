/**
 * @file hanalyze.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Analyze functionality of headless mode.
 * @version 1.0.0
 * @date 2024-01-15
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

#ifndef GAMESMANONE_CORE_HEADLESS_HANALYZE_H_
#define GAMESMANONE_CORE_HEADLESS_HANALYZE_H_

#include <stdbool.h>  // bool

#include "core/types/gamesman_types.h"

/**
 * @brief Analyzes the variant VARIANT_ID of game GAME_NAME.
 *
 * @param game_name Name of the game internal to Gamesman.
 * @param variant_id Variant index of the game. The default variant will be
 * analyzed instead set to a negative value.
 * @param data_path Path to the `data` directory. The default data path will be
 * used if set to NULL.
 * @param force If true, the system will force re-analyze the given game
 * regardless of the current analysis status.
 * @param verbose May take values 0, 1, or 2. If set to 0, no output will be
 * produced to stdout. Set to 1 for default output level. Set to 2 for more
 * detailed output.
 * @return 0 on success, non-zero error code otherwise.
 */
int HeadlessAnalyze(ReadOnlyString game_name, int variant_id,
                    ReadOnlyString data_path, bool force, int verbose);

#endif  // GAMESMANONE_CORE_HEADLESS_HANALYZE_H_

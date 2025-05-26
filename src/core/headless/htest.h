/**
 * @file htest.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Game testing functionality of headless mode.
 * @version 1.0.0
 * @date 2025-05-10
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

#ifndef GAMESMANONE_CORE_HEADLESS_HTEST_H_
#define GAMESMANONE_CORE_HEADLESS_HTEST_H_

#include "core/types/gamesman_types.h"

/**
 * @brief Tests the game of name \p game_name and variant index \p variant_id
 * using \p seed as the seed for PRNGs if needed.
 *
 * @param game_name Name of the game used internally by GAMESMAN.
 * @param variant_id Index of the variant to solve for. Pass a negative value to
 * test the default variant.
 * @param seed Seed for PRNGs.
 * @param verbose Currently has no effect regardless of the value passed in.
 * @return kNoError if all tests are passed,
 * @return non-zero error code otherwise.
 */
int HeadlessTest(ReadOnlyString game_name, int variant_id, long seed,
                 int verbose);

#endif  // GAMESMANONE_CORE_HEADLESS_HTEST_H_

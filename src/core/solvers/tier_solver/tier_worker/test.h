/**
 * @file test.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Tier worker testing module.
 * @version 1.1.0
 * @date 2024-09-13
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_TEST_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_TEST_H_

#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

/**
 * @brief Tests the implementation of the \p api functions for positions in the
 * given \p tier.
 *
 * @param api Game-specific tier solver API functions.
 * @param tier Tier to test.
 * @param parent_tiers Parent tiers of \p tier.
 * @param seed Seed for internal pseudorandom number generator.
 * @param test_size Maximum number of positions to test in the given \p tier.
 * @return One of the values in the \c TierSolverTestErrors enum. See
 * \c tier_solver.h for details.
 */
int TierWorkerTestInternal(const TierSolverApi *api, Tier tier,
                           const TierArray *parent_tiers, long seed,
                           int64_t test_size);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_TEST_H_

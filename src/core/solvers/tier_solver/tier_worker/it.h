/**
 * @file it.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Immediate transition tier worker algorithm.
 * @version 1.0.0
 * @date 2024-09-05
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_IT_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_IT_H_

#include <stdbool.h>  // bool

#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

/**
 * @brief Solves \p tier using the immediate transition algorithm given \p api.
 *
 * @param api Game-specific tier solver API functions.
 * @param tier Tier to solve.
 * @param memlimit Maximum amount of heap memory that can be used in bytes.
 * @param force Set this to true to force re-solve \p tier.
 * @param compare Set this to true to compare newly solved results to the ones
 * stored in the reference database, which is assumed to be initialized.
 * @param solved (Output parameter) If non-NULL, its value will be set to
 * \c true if \p tier is actually solved, or \p false if \p tier is loaded from
 * an existing database.
 * @return kNoError on success, or
 * @return non-zero error code otherwise.
 */
int TierWorkerSolveITInternal(const TierSolverApi *api, Tier tier,
                              intptr_t memlimit, bool force, bool compare,
                              bool *solved);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_IT_H_

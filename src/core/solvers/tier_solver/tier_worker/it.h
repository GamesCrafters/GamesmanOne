/**
 * @file it.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Immediate transition tier worker algorithm.
 * @version 1.1.5
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_IT_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_IT_H_

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t

#include "core/solvers/tier_solver/tier_solver.h"
#include "core/solvers/tier_solver/tier_worker.h"
#include "core/types/gamesman_types.h"

/**
 * @brief Solves \p tier using the immediate transition algorithm given \p api.
 *
 * @param api Game-specific tier solver API functions.
 * @param tier Tier to solve.
 * @param options Pointer to a \c TierWorkerSolveOptions object which contains
 * the options.
 * @param solved (Output parameter) If non-NULL, its value will be set to
 * \c true if \p tier is actually solved, or \p false if \p tier is loaded from
 * an existing database.
 * @return kNoError on success, or
 * @return non-zero error code otherwise.
 */
int TierWorkerSolveITInternal(const TierSolverApi *api, Tier tier,
                              const TierWorkerSolveOptions *options,
                              bool *solved);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_IT_H_

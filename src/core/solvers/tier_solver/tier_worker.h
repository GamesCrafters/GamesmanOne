/**
 * @file tier_worker.h
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Separated functions for
 * solving of a single tier into its own module, implemented multithreading
 * using OpenMP, and reformatted functions for readability.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Worker module for the Loopy Tier Solver.
 * @version 1.2.1
 * @date 2024-02-29
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // uint64_t

#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

/**
 * @brief Initializes the Tier Worker Module using the given API functions.
 *
 * @param api Game-specific implementation of the Tier Solver API functions.
 */
void TierWorkerInit(const TierSolverApi *api);

/**
 * @brief Solves the given TIER.
 *
 * @param tier Tier to solve.
 * @param force If set to true, the Module will perform the solving process
 * regardless of the current database status. Otherwise, the solving process is
 * skipped if the Module believes that TIER has been correctly solved already.
 * @param solved If not NULL, a truth value indicating whether the given TIER is
 * actually solved instead of loaded from the existing database will be stored
 * in this variable.
 * @return 0 on success, non-zero error code otherwise.
 */
int TierWorkerSolve(Tier tier, bool force, bool *solved);

#ifdef USE_MPI
int TierWorkerMpiServe(void);
#endif  // USE_MPI

int TierWorkerTest(Tier tier, const TierArray *parent_tiers, long seed);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_H_

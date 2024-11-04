/**
 * @file tier_manager.h
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Separated functions that
 * handle tier graph traversal and management into its own module, and
 * redesigned retrograde tier analysis process to enable concurrent solving
 * of multiple tiers.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Manager module of the Loopy Tier Solver.
 *
 * @details The tier manager module is responsible for scanning, validating, and
 * creating the tier graph in memory, keeping track of solved and solvable
 * tiers, and dispatching jobs to the tier worker module.
 * @version 1.3.0
 * @date 2024-07-11
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_MANAGER_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_MANAGER_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/solvers/tier_solver/tier_solver.h"

/**
 * @brief Creates and solves the tier graph.
 *
 * @param api Tier solver API functions implemented by the current Game.
 * @param force If set to true, the solver will solve each tier regardless of
 * the current database status. Otherwise, the solving stage is skipped if Tier
 * Manager believes that the given tier has been correctly solved already.
 * @param verbose Set to 0 for quiet (only error messages will be printed,) 1
 * for default, and 2 for verbose.
 * @return 0 on success, non-zero error code otherwise.
 */
int TierManagerSolve(const TierSolverApi *api, bool force, int verbose);

/**
 * @brief Creates and analyzes the tier graph.
 *
 * @param api Tier solver API functions implemented by the current Game.
 * @param force If set to true, the analyzer will analyze each tier regardless
 * of the current analysis status. Otherwise, the analyzing stage is skipped if
 * Tier Manager believes that the given tier has been correctly analyzed
 * already.
 * @param verbose Set to 0 for quiet (only error messages will be printed,) 1
 * for default, and 2 for verbose.
 * @return 0 on success, non-zero error code otherwise.
 */
int TierManagerAnalyze(const TierSolverApi *api, bool force, int verbose);

/**
 * @brief Tests the given tier solver API implementation using the given SEED
 * for random number generation.
 *
 * @param api Tier solver API to test.
 * @param seed Seed for random number generation.
 * @param test_size Maximum number of positions to test in each tier.
 * @return 0 on success, or
 * @return one of the values from TierSolverTestErrors defined in tier_solver.h.
 */
int TierManagerTest(const TierSolverApi *api, long seed, int64_t test_size);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_MANAGER_H_

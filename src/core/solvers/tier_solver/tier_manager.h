/**
 * @file tier_manager.h
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Separated functions that
 * handle tier graph traversal and management into its own module, and
 * redesigned retrograde tier analysis process to enable concurrent solving
 * of multiple tiers.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Manager module of the Loopy Tier Solver.
 *
 * @details The tier manager module is responsible for scanning, validating, and
 * creating the tier graph in memory, keeping track of solved and solvable
 * tiers, and dispatching jobs to the tier worker module.
 * @version 2.0.0
 * @date 2026-06-04
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

#include "core/solvers/tier_solver/tier_solver.h"

/**
 * @brief Creates and solves the tier graph.
 *
 * @param api Tier solver API functions implemented by the current Game.
 * @param options Non-null pointer to a \c TierSolverSolveOptions object which
 * contains the solving options.
 * @return 0 on success, non-zero error code otherwise.
 */
int TierManagerSolve(const TierSolverApi *api,
                     const TierSolverSolveOptions *options);

/**
 * @brief Creates and analyzes the tier graph.
 *
 * @param api Tier solver API functions implemented by the current Game.
 * @param options Non-null pointer to a \c TierSolverAnalyzeOptions object which
 * contains the analyzing options.
 * @return 0 on success, non-zero error code otherwise.
 */
int TierManagerAnalyze(const TierSolverApi *api,
                       const TierSolverAnalyzeOptions *options);

/**
 * @brief Tests the given tier solver API implementation using the given SEED
 * for random number generation.
 *
 * @param api Tier solver API to test.
 * @param options Non-null pointer to a \c TierSolverTestOptions object which
 * contains the test options.
 * @return 0 on success, or
 * @return one of the values from TierSolverTestErrors defined in tier_solver.h.
 */
int TierManagerTest(const TierSolverApi *api,
                    const TierSolverTestOptions *options);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_MANAGER_H_

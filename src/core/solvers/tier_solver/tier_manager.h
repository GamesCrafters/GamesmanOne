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
 *
 * @version 1.1
 * @date 2023-10-18
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

#ifndef GAMESMANEXPERIMENT_CORE_SOLVERS_TIER_SOLVER_TIER_MANAGER_H_
#define GAMESMANEXPERIMENT_CORE_SOLVERS_TIER_SOLVER_TIER_MANAGER_H_

#include <stdbool.h>  // bool

#include "core/solvers/tier_solver/tier_solver.h"

/**
 * @brief Creates and solves the tier tree.
 *
 * @param api Tier solver API functions implemented by the current Game.
 * @param force If set to true, the solver will solve each tier regardless of
 * the current database status. Otherwise, the solving stage is skipped if Tier
 * Manager believes that the given tier has been correctly solved already.
 * @return 0 on success, non-zero error code otherwise.
 */
int TierManagerSolve(const TierSolverApi *api, bool force);

/**
 * @brief Creates and analyzes the tier tree.
 *
 * @param api Tier solver API functions implemented by the current Game.
 * @param force If set to true, the analyzer will analyze each tier regardless
 * of the current analysis status. Otherwise, the analyzing stage is skipped if
 * Tier Manager believes that the given tier has been correctly analyzed
 * already.
 * @return 0 on success, non-zero error code otherwise.
 */
int TierManagerAnalyze(const TierSolverApi *api, bool force);

#endif  // GAMESMANEXPERIMENT_CORE_SOLVERS_TIER_SOLVER_TIER_MANAGER_H_

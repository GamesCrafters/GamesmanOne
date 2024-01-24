/**
 * @file solver_manager.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Solver manager module which handles solver loading, solving, and
 * database status checking.
 *
 * @details One of the key assumptions made by GAMESMAN is that "no more than
 * one game can be loaded at the same time," because a user can always run
 * multiple instances of GAMESMAN if they wish to solve/play multiple games
 * simultaneously. As a result, no more than one solver or database can be
 * loaded at the same time. This module handles the loading and deallocation
 * of THE ONE solver used by the current GAMESMAN instance.
 *
 * @version 1.1.0
 * @date 2024-01-08
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

#ifndef GAMESMANONE_CORE_SOLVERS_SOLVER_MANAGER_H_
#define GAMESMANONE_CORE_SOLVERS_SOLVER_MANAGER_H_

#include "core/types/gamesman_types.h"

/**
 * @brief Initializes the Solver specified by the current game loaded in the
 * Game Manager Module, and finalizes the previous solver.
 * @details Current implementation only supports loading one game at a time.
 *
 * @param data_path Absolute or relative path to the data directory if non-NULL.
 * The default path "data" will be used if set to NULL.
 * @return 0 on success, non-zero error code otherwise.
 */
int SolverManagerInit(ReadOnlyString data_path);

/**
 * @brief Returns the solver status of the current game.
 *
 * @note Assumes a game together with its solver have been loaded using the
 * SolverManagerInit() function. Results in undefined behavior otherwise.
 *
 * @return Status encoded as an int. The encoding is specific to each solver
 * module.
 */
int SolverManagerGetSolverStatus(void);

/**
 * @brief Solves the current game.
 *
 * @param aux Auxiliary parameter.
 * @return 0 on success, non-zero error code otherwise.
 */
int SolverManagerSolve(void *aux);

/**
 * @brief Analyzes the current game.
 *
 * @param aux Auxiliary parameter.
 * @return 0 on success, non-zero error code otherwise.
 */
int SolverManagerAnalyze(void *aux);

/**
 * @brief Probes and returns the value of the given TIER_POSITION.
 *
 * @note Assumes the solver manager is initialized with the SolverManagerInit
 * function. Results in undefined behavior if called before the solver manager
 * module is initialized.
 *
 * @param tier_position Tier position to probe.
 * @return Value of the given TIER_POSITION.
 */
Value SolverManagerGetValue(TierPosition tier_position);

/**
 * @brief Probes and returns the remoteness of the given TIER_POSITION.
 *
 * @note Assumes the solver manager is initialized with the SolverManagerInit
 * function. Results in undefined behavior if called before the solver manager
 * module is initialized.
 *
 * @param tier_position Tier position to probe.
 * @return Remoteness of the given TIER_POSITION.
 */
int SolverManagerGetRemoteness(TierPosition tier_position);

#endif  // GAMESMANONE_CORE_SOLVERS_SOLVER_MANAGER_H_

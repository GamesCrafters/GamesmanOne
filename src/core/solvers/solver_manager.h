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
 * @version 1.0
 * @date 2023-08-19
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

#ifndef GAMESMANEXPERIMENT_CORE_SOLVERS_SOLVER_MANAGER_H_
#define GAMESMANEXPERIMENT_CORE_SOLVERS_SOLVER_MANAGER_H_

#include "core/gamesman_types.h"

/**
 * @brief Initializes the Solver specified by GAME, finalizing any games/solvers
 * that were previously loaded.
 *
 * @param game The one and only game that we are loading into GAMESMAN.
 * @return 0 on success, non-zero error code otherwise.
 */
int SolverManagerInitSolver(const Game *game);

/**
 * @brief Returns the solver status of the current game.
 *
 * @note Assumes a game together with its solver have been loaded using the
 * SolverManagerInitSolver() function. Results in undefined behavior otherwise.
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

int SolverManagerAnalyze(void *aux);

#endif  // GAMESMANEXPERIMENT_CORE_SOLVERS_SOLVER_MANAGER_H_

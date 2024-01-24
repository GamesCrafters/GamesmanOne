/**
 * @file solver.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The generic solver type.
 *
 * @version 1.0.0
 * @date 2024-01-23
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

#ifndef GAMESMANONE_CORE_TYPES_SOLVER_SOLVER_H
#define GAMESMANONE_CORE_TYPES_SOLVER_SOLVER_H

#include "core/types/base.h"
#include "core/types/solver/solver_config.h"

enum SolverConstants {
    kSolverNameLengthMax = 63,
};

/**
 * @brief Generic Solver type.
 * @note To implement a new Solver module, set the name of the new Solver and
 * each member function pointer to a module-specific function.
 *
 * @note A Solver can either be a regular solver or a tier solver. The actual
 * behavior and requirements of the solver is decided by the Solver and
 * reflected on its SOLVER_API, which is a custom struct defined in the Solver
 * module and implemented by the game developer. The game developer should
 * decide which Solver to use and implement its required Solver API functions.
 */
typedef struct Solver {
    /** Human-readable name of the solver. */
    char name[kSolverNameLengthMax + 1];

    /**
     * @brief Initializes the Solver.
     *
     * @param game_name Game name used internally by GAMESMAN.
     * @param variant Index of the current game variant.
     * @param solver_api Pointer to a struct that contains the implemented
     * Solver API functions. The game developer is responsible for using the
     * correct type of Solver API that applies to the current Solver,
     * implementing and setting up required API functions, and casting the
     * pointer to a constant generic pointer (const void *) type.
     * @param data_path Absolute or relative path to the data directory if
     * non-NULL. The default path "data" will be used if set to NULL.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Init)(ReadOnlyString game_name, int variant, const void *solver_api,
                ReadOnlyString data_path);

    /**
     * @brief Finalizes the Solver, freeing all allocated memory.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Finalize)(void);

    /**
     * @brief Solves the current game and stores the result if a Database is set
     * for the current Solver.
     *
     * @param aux Auxiliary parameter.
     */
    int (*Solve)(void *aux);

    /**
     * @brief Analyzes the current game.
     *
     * @param aux Auxiliary parameter.
     */
    int (*Analyze)(void *aux);

    /**
     * @brief Returns the solving status of the current game.
     *
     * @return Status code as defined by the actual Solver module.
     */
    int (*GetStatus)(void);

    /** @brief Returns the current configuration of this Solver. */
    const SolverConfig *(*GetCurrentConfig)(void);

    /**
     * @brief Sets the solver option with index OPTION to the choice of index
     * SELECTION.
     *
     * @param option Solver option index.
     * @param selection Choice index to select.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*SetOption)(int option, int selection);

    /**
     * @brief Probes and returns the value of the given TIER_POSITION. Results
     * in undefined behavior if the TIER_POSITION has not been solved, or if
     * TIER_POSITION is invalid or unreachable.
     *
     * @param tier_position Tier position to probe.
     *
     * @return Value of TIER_POSITION.
     */
    Value (*GetValue)(TierPosition tier_position);

    /**
     * @brief Probes and returns the remoteness of the given TIER_POSITION.
     * Results in undefined behavior if the TIER_POSITION has not been solved,
     * or if TIER_POSITION is invalid or unreachable.
     *
     * @param tier_position Tier position to probe.
     *
     * @return Remoteness of TIER_POSITION.
     */
    int (*GetRemoteness)(TierPosition tier_position);
} Solver;

#endif  // GAMESMANONE_CORE_TYPES_SOLVER_SOLVER_H

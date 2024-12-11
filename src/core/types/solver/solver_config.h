/**
 * @file solver_config.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Configuration of a generic solver.
 * @version 0.1.1
 * @date 2024-12-10
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

#ifndef GAMESMANONE_CORE_TYPES_SOLVER_SOLVER_CONFIG_H_
#define GAMESMANONE_CORE_TYPES_SOLVER_SOLVER_CONFIG_H_

#include "core/types/solver/solver_option.h"

/** @brief Solver configuration as an array of selected solver options. */
typedef struct SolverConfig {
    /**
     * Zero-terminated array of solver options. The user of this struct must
     * make sure that the last item in this array is completely zeroed out.
     */
    const SolverOption *options;

    /**
     * Array of selected choice indices to each option. Zero-terminated and
     * aligned to the options array (same number of options and selections.)
     */
    const int *selections;
} SolverConfig;

#endif  // GAMESMANONE_CORE_TYPES_SOLVER_SOLVER_CONFIG_H_

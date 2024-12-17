/**
 * @file solver_option.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Options for a generic solver.
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

#ifndef GAMESMANONE_CORE_TYPES_SOLVER_SOLVER_OPTION_H_
#define GAMESMANONE_CORE_TYPES_SOLVER_SOLVER_OPTION_H_

#include "core/types/base.h"

enum SolverOptionConstants {
    kSolverOptionNameLengthMax = 63,
    kSolverOptionChoiceNameLengthMax = 127,
};

/** @brief Solver option for display in GAMESMAN interactive mode. */
typedef struct SolverOption {
    /** Human-readable name of the option. */
    char name[kSolverOptionNameLengthMax + 1];

    /** Number of choices associated with the option. */
    int num_choices;

    /** An array of strings, where each string is the name of a choice. Length =
     * num_choices.  */
    const ConstantReadOnlyString *choices;
} SolverOption;

#endif  // GAMESMANONE_CORE_TYPES_SOLVER_SOLVER_OPTION_H_

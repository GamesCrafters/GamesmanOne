/**
 * @file tier_analyzer.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Analyzer module for the Loopy Tier Solver.
 * @version 1.2.3
 * @date 2024-12-22
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_ANALYZER_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_ANALYZER_H_

#include <stdbool.h>  // bool

#include "core/analysis/analysis.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

/**
 * @brief Initializes the Tier Analyzer Module using the given API functions.
 *
 * @param api Game-specific implementation of the Tier Solver API functions.
 */
void TierAnalyzerInit(const TierSolverApi *api);

/**
 * @brief Analyzes the given TIER.
 *
 * @param dest Preallocated space for analysis output.
 * @param tier Tier to analyze.
 * @param force If set to true, the Module will discover the tier regardless of
 * the current analysis status. Otherwise, the discovering process is skipped if
 * the Module believes that TIER has been correctly analyzed already.
 * @return 0 on success, non-zero error code otherwise.
 */
int TierAnalyzerAnalyze(Analysis *dest, Tier tier, bool force);

/**
 * @brief Finalizes the Tier Analyzer Module.
 */
void TierAnalyzerFinalize(void);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_ANALYZER_H_

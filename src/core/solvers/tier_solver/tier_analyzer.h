#ifndef GAMESMANEXPERIMENT_CORE_SOLVERS_TIER_SOLVER_TIER_ANALYZER_H_
#define GAMESMANEXPERIMENT_CORE_SOLVERS_TIER_SOLVER_TIER_ANALYZER_H_

#include <stdbool.h>  // bool

#include "core/analysis/analysis.h"
#include "core/gamesman_types.h"
#include "core/solvers/tier_solver/tier_solver.h"

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
int TierAnalyzerDiscover(Analysis *dest, Tier tier, bool force);

void TierAnalyzerFinalize(void);

#endif  // GAMESMANEXPERIMENT_CORE_SOLVERS_TIER_SOLVER_TIER_ANALYZER_H_

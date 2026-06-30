/**
 * @file bi.c
 * @author Max Delgadillo: designed and implemented the original version
 * of the backward induction algorithm (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Implemented multiple memory
 * saving strategies and added multithreading support.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Backward induction tier worker algorithm implementation.
 * @version 1.1.5
 * @date 2025-05-27
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

#include "core/solvers/tier_solver/tier_worker/bi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/db/db_manager.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/solvers/tier_solver/tier_worker/backward_induction/frontier_percolation.h"
#include "core/solvers/tier_solver/tier_worker/backward_induction/frontierless.h"
#include "core/solvers/tier_solver/tier_worker/backward_induction/one_bit.h"
#include "core/solvers/tier_solver/tier_worker/backward_induction/types.h"
#include "core/types/base.h"
#include "core/types/database/database.h"
#include "core/types/gamesman_error.h"

static bool GetParentsAvailable(const TierSolverApi *api) {
    return api->GetCanonicalParentPositions != NULL;
}

static BackwardInductionStrategy BestStrategy(const TierSolverApi *api,
                                              Tier tier, size_t memlimit) {
    int64_t this_tier_size = api->GetTierSize(tier);
    Tier child_tiers[kTierSolverNumChildTiersMax];
    int num_child_tiers = api->GetChildTiers(tier, child_tiers);
    int64_t tier_children_total_size = 0;
    for (int i = 0; i < num_child_tiers; ++i) {
        tier_children_total_size += api->GetTierSize(child_tiers[i]);
    }

    if (TierWorkerBIFrontierlessMemReq(tier, this_tier_size) <= memlimit) {
        return kFrontierPercolation;
    } else if (!GetParentsAvailable(api)) {
        return kUnsolvable;
    } else if (OneBitMemReq(this_tier_size + tier_children_total_size) <=
               memlimit) {
        return kOneBit;
    }

    return kUnsolvable;
}

// -----------------------------------------------------------------------------
// ------------------------ TierWorkerBackwardInduction ------------------------
// -----------------------------------------------------------------------------

int TierWorkerBackwardInduction(const TierSolverApi *api, int64_t db_chunk_size,
                                Tier tier,
                                const TierSolverSolveOptions *options,
                                bool *solved) {
    if (solved != NULL) *solved = false;

    // If we are not force-resolving and the tier has been solved, skip it.
    if (!options->force && DbManagerTierStatus(tier) == kDbTierStatusSolved) {
        return kNoError;
    }

    // Analyze memory usage and decide the best solving strategy.
    BackwardInductionStrategy strategy =
        BestStrategy(api, tier, options->memlimit);

    // If we don't have enough memory to solve the tier, report failure.
    if (strategy == kUnsolvable) return kMallocFailureError;

    int error = kNoError;
    switch (strategy) {
        case kFrontierPercolation:
        case kFrontierless:
            error = TierWorkerBIFrontierPercolation(api, db_chunk_size, tier,
                                                    options, solved);

            // If succeeded with either strategy, or failed not because of OOM,
            // return the error code because it doesn't make sense to try again.
            if (error != kMallocFailureError) return error;
            // Else: frontier percolation ran out of memory.

            // The frontier-less method requires an implementation of the
            // GetCanonicalParentPositions function.
            // If the game does not implement GetCanonicalParentPositions, then
            // the game developers should first consider implementing that to
            // reduce memory usage.
            if (!GetParentsAvailable(api)) return kMallocFailureError;

            // Otherwise, use the frontier-less approach instead.
            return TierWorkerBIFrontierless(api, db_chunk_size, tier, solved);

        case kOneBit:
            return TierWorkerBIOneBit(api, db_chunk_size, tier, options,
                                      solved);
            break;

        default:
            error = kMallocFailureError;
    }

    return error;
}

/**
 * @file tier_worker.c
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Separated functions for
 * solving of a single tier into its own module, implemented multithreading
 * using OpenMP, and reformatted functions for readability. New algorithm
 * using value iteration.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the worker module for the Loopy Tier Solver.
 * @version 2.0.0
 * @date 2025-05-11
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

#include "core/solvers/tier_solver/tier_worker.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t
#include <stdint.h>   // int64_t

#include "core/misc.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/solvers/tier_solver/tier_worker/bi.h"
#include "core/solvers/tier_solver/tier_worker/it.h"
#include "core/solvers/tier_solver/tier_worker/test.h"
#include "core/solvers/tier_solver/tier_worker/vi.h"
#include "core/types/gamesman_types.h"

static const TierSolverApi *api_internal;
static int64_t current_db_chunk_size;
static size_t mem;

#ifdef USE_MPI
#include <unistd.h>  // sleep

#include "core/solvers/tier_solver/tier_mpi.h"
#endif  // USE_MPI

// ============================== TierWorkerInit ==============================

void TierWorkerInit(const TierSolverApi *api, int64_t db_chunk_size,
                    size_t memlimit) {
    assert(db_chunk_size > 0);
    api_internal = api;
    current_db_chunk_size = db_chunk_size;
    mem = memlimit;
}

// =========================== GetMethodForTierType ===========================

int GetMethodForTierType(TierType type) {
    switch (type) {
        case kTierTypeImmediateTransition:
            return kTierWorkerSolveMethodImmediateTransition;

        case kTierTypeLoopFree:  // TODO: implement a more efficient method.
            return kTierWorkerSolveMethodBackwardInduction;

        case kTierTypeLoopy:
            return kTierWorkerSolveMethodBackwardInduction;
    }

    NotReached("GetMethodForTierType: unknown tier type");
    return -1;  // Never reached.
}

// ============================== TierWorkerSolve ==============================

const TierWorkerSolveOptions kDefaultTierWorkerSolveOptions = {
    .compare = false,
    .force = false,
    .verbose = 1,
};

int TierWorkerSolve(int method, Tier tier,
                    const TierWorkerSolveOptions *options, bool *solved) {
    if (options == NULL) options = &kDefaultTierWorkerSolveOptions;
    switch (method) {
        case kTierWorkerSolveMethodImmediateTransition:
            return TierWorkerSolveITInternal(api_internal, tier, mem, options,
                                             solved);
        case kTierWorkerSolveMethodBackwardInduction:
            return TierWorkerSolveBIInternal(
                api_internal, current_db_chunk_size, tier, options, solved);
        case kTierWorkerSolveMethodValueIteration:
            return TierWorkerSolveVIInternal(api_internal, tier, options,
                                             solved);
        default:
            break;
    }

    // Unknown method.
    return kRuntimeError;
}

#ifdef USE_MPI
int TierWorkerMpiServe(void) {
    TierMpiWorkerSendCheck();
    while (true) {
        TierMpiManagerMessage msg;
        TierMpiWorkerRecv(&msg);

        if (msg.command == kTierMpiCommandSleep) {
            // No work to do. Wait for one second and check again.
            sleep(1);  // NOLINT(concurrency-mt-unsafe)
            TierMpiWorkerSendCheck();
        } else if (msg.command == kTierMpiCommandTerminate) {
            break;
        } else {
            TierWorkerSolveOptions options = {
                .compare = false,
                .force = (msg.command == kTierMpiCommandForceSolve),
                .verbose = false,
            };
            bool solved;
            TierType type = api_internal->GetTierType(msg.tier);
            int error = TierWorkerSolve(type, msg.tier, &options, &solved);
            if (error != kNoError) {
                TierMpiWorkerSendReportError(error);
            } else if (solved) {
                TierMpiWorkerSendReportSolved();
            } else {
                TierMpiWorkerSendReportLoaded();
            }
        }
    }

    return kNoError;
}
#endif  // USE_MPI

int TierWorkerTest(Tier tier, const TierArray *parent_tiers, long seed,
                   int64_t test_size, TierWorkerTestStackBufferStat *stat) {
    return TierWorkerTestInternal(api_internal, tier, parent_tiers, seed,
                                  test_size, stat);
}

/**
 * @file tier_worker.c
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Separated functions for
 * solving of a single tier into its own module, implemented multithreading
 * using OpenMP, and reformatted functions for readability.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the worker module for the Loopy Tier Solver.
 * @version 1.2.1
 * @date 2024-02-29
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
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t, uint64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // calloc, free
#include <string.h>   // memcpy

#include "core/constants.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/solvers/tier_solver/tier_worker/bi.h"
#include "core/solvers/tier_solver/tier_worker/test.h"
#include "core/solvers/tier_solver/tier_worker/vi.h"
#include "core/types/gamesman_types.h"

static const TierSolverApi *api_internal;
static int64_t current_db_chunk_size;

#ifdef USE_MPI
#include <unistd.h>  // sleep

#include "core/solvers/tier_solver/tier_mpi.h"
#endif  // USE_MPI

// -----------------------------------------------------------------------------

void TierWorkerInit(const TierSolverApi *api, int64_t db_chunk_size) {
    api_internal = api;
    current_db_chunk_size = db_chunk_size;
    assert(current_db_chunk_size > 0);
}

int TierWorkerSolve(int method, Tier tier, bool force, bool compare,
                    bool *solved) {
    switch (method) {
        case kTierWorkerSolveMethodBackwardInduction:
            return TierWorkerSolveBIInternal(api_internal,
                                             current_db_chunk_size, tier, force,
                                             compare, solved);

        case kTierWorkerSolveMethodValueIteration:
            return TierWorkerSolveVIInternal(api_internal,
                                             current_db_chunk_size, tier, force,
                                             compare, solved);
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
            sleep(1);
            TierMpiWorkerSendCheck();
        } else if (msg.command == kTierMpiCommandTerminate) {
            break;
        } else {
            bool force = (msg.command == kTierMpiCommandForceSolve);
            bool solved;
            int error = TierWorkerSolve(kTierWorkerSolveMethodBackwardInduction,
                                        msg.tier, force, false, &solved);
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

int TierWorkerTest(Tier tier, const TierArray *parent_tiers, long seed) {
    return TierWorkerTestInternal(api_internal, tier, parent_tiers, seed);
}

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
#include "core/solvers/tier_solver/tier_worker/vi.h"
#include "core/types/gamesman_types.h"
#include "libs/mt19937/mt19937-64.h"

// Copy of the API functions from tier_manager. Cannot use a reference here
// because we need to create/modify some of the functions.
static TierSolverApi current_api;

static int64_t current_db_chunk_size;

#ifdef USE_MPI
#include <unistd.h>  // sleep

#include "core/solvers/tier_solver/tier_mpi.h"
#endif  // USE_MPI

static bool TestShouldSkip(Tier tier, Position position);
static int TestChildPositions(Tier tier, Position position);
static int TestChildToParentMatching(Tier tier, Position position);
static int TestParentToChildMatching(Tier tier, Position position,
                                     const TierArray *parent_tiers);
static int TestPrintError(Tier tier, Position position);

// -----------------------------------------------------------------------------

void TierWorkerInit(const TierSolverApi *api, int64_t db_chunk_size) {
    memcpy(&current_api, api, sizeof(current_api));
    current_db_chunk_size = db_chunk_size;
    assert(current_db_chunk_size > 0);
}

int TierWorkerSolve(int method, Tier tier, bool force, bool compare,
                    bool *solved) {
    switch (method) {
        case kTierWorkerSolveMethodBackwardInduction:
            return TierWorkerSolveBIInternal(&current_api,
                                             current_db_chunk_size, tier, force,
                                             compare, solved);

        case kTierWorkerSolveMethodValueIteration:
            return TierWorkerSolveVIInternal(&current_api,
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

static int TestTierSymmetryRemoval(Tier tier, Position position,
                                   Tier canonical_tier) {
    Position (*ApplySymm)(TierPosition, Tier) =
        current_api.GetPositionInSymmetricTier;

    TierPosition self = {.tier = tier, .position = position};
    TierPosition symm = {.tier = canonical_tier};
    symm.position = ApplySymm(self, canonical_tier);

    // Test if getting symmetric position from the same tier returns the
    // position itself.
    Position self_in_self_tier = ApplySymm(self, self.tier);
    Position symm_in_symm_tier = ApplySymm(symm, symm.tier);
    if (self_in_self_tier != self.position) {
        return kTierSolverTestTierSymmetrySelfMappingError;
    } else if (symm_in_symm_tier != symm.position) {
        return kTierSolverTestTierSymmetrySelfMappingError;
    }

    // Skip the next test if both tiers are the same.
    if (tier == canonical_tier) return kTierSolverTestNoError;

    // Test if applying the symmetry twice returns the same position.
    Position self_in_symm_tier = ApplySymm(self, symm.tier);
    Position symm_in_self_tier = ApplySymm(symm, self.tier);
    TierPosition self_in_symm_tier_tier_position = {
        .tier = symm.tier, .position = self_in_symm_tier};
    TierPosition symm_in_self_tier_tier_position = {
        .tier = self.tier, .position = symm_in_self_tier};
    Position new_self = ApplySymm(self_in_symm_tier_tier_position, self.tier);
    Position new_symm = ApplySymm(symm_in_self_tier_tier_position, symm.tier);
    if (new_self != self.position) {
        return kTierSolverTestTierSymmetryInconsistentError;
    } else if (new_symm != symm.position) {
        return kTierSolverTestTierSymmetryInconsistentError;
    }

    return kTierSolverTestNoError;
}

int TierWorkerTest(Tier tier, const TierArray *parent_tiers, long seed) {
    static const int64_t kTestSizeMax = 1000;
    init_genrand64(seed);

    int64_t tier_size = current_api.GetTierSize(tier);
    bool random_test = tier_size > kTestSizeMax;
    int64_t test_size = random_test ? kTestSizeMax : tier_size;
    Tier canonical_tier = current_api.GetCanonicalTier(tier);

    for (int64_t i = 0; i < test_size; ++i) {
        Position position = random_test ? genrand64_int63() % tier_size : i;
        if (TestShouldSkip(tier, position)) continue;

        // Check tier symmetry removal implementation.
        int error = TestTierSymmetryRemoval(tier, position, canonical_tier);
        if (error != kTierSolverTestNoError) {
            TestPrintError(tier, position);
            return error;
        }

        // Check if all child positions are legal.
        error = TestChildPositions(tier, position);
        if (error != kTierSolverTestNoError) {
            TestPrintError(tier, position);
            return error;
        }

        // Perform the following tests only if current game variant implements
        // its own GetCanonicalParentPositions.
        if (current_api.GetCanonicalParentPositions != NULL) {
            // Check if all child positions of the current position has the
            // current position as one of their parents.
            error = TestChildToParentMatching(tier, position);
            if (error != kTierSolverTestNoError) {
                TestPrintError(tier, position);
                return error;
            }

            // Check if all parent positions of the current position has the
            // current position as one of their children.
            error = TestParentToChildMatching(tier, position, parent_tiers);
            if (error != kTierSolverTestNoError) {
                TestPrintError(tier, position);
                return error;
            }
        }
    }

    return kTierSolverTestNoError;
}

// -----------------------------------------------------------------------------

static bool TestShouldSkip(Tier tier, Position position) {
    TierPosition tier_position = {.tier = tier, .position = position};
    if (!current_api.IsLegalPosition(tier_position)) return true;
    if (current_api.Primitive(tier_position) != kUndecided) return true;

    return false;
}

static int TestChildPositions(Tier tier, Position position) {
    TierPosition parent = {.tier = tier, .position = position};
    TierPositionArray children = current_api.GetCanonicalChildPositions(parent);
    int error = kTierSolverTestNoError;
    for (int64_t i = 0; i < children.size; ++i) {
        TierPosition child = children.array[i];
        if (child.position < 0) {
            error = kTierSolverTestIllegalChildError;
            break;
        } else if (child.position >= current_api.GetTierSize(child.tier)) {
            error = kTierSolverTestIllegalChildError;
            break;
        } else if (!current_api.IsLegalPosition(child)) {
            error = kTierSolverTestIllegalChildError;
            break;
        }
    }

    return error;
}

static int TestChildToParentMatching(Tier tier, Position position) {
    TierPosition parent = {.tier = tier, .position = position};
    TierPosition canonical_parent = parent;
    canonical_parent.position =
        current_api.GetCanonicalPosition(canonical_parent);
    TierPositionArray children = current_api.GetCanonicalChildPositions(parent);
    int error = kTierSolverTestNoError;
    for (int64_t i = 0; i < children.size; ++i) {
        // Check if all child positions have parent as one of their parents.
        TierPosition child = children.array[i];
        PositionArray parents =
            current_api.GetCanonicalParentPositions(child, tier);
        if (!PositionArrayContains(&parents, canonical_parent.position)) {
            error = kTierSolverTestChildParentMismatchError;
        }

        PositionArrayDestroy(&parents);
        if (error != kTierSolverTestNoError) break;
    }
    TierPositionArrayDestroy(&children);

    return error;
}

static int TestParentToChildMatching(Tier tier, Position position,
                                     const TierArray *parent_tiers) {
    TierPosition child = {.tier = tier, .position = position};
    TierPosition canonical_child = child;
    canonical_child.position =
        current_api.GetCanonicalPosition(canonical_child);

    int error = kTierSolverTestNoError;
    for (int64_t i = 0; i < parent_tiers->size; ++i) {
        Tier parent_tier = parent_tiers->array[i];
        PositionArray parents = current_api.GetCanonicalParentPositions(
            canonical_child, parent_tier);
        for (int64_t j = 0; j < parents.size; ++j) {
            // Skip illegal and primitive parent positions as they are also
            // skipped in solving.
            TierPosition parent = {.tier = parent_tier,
                                   .position = parents.array[j]};
            if (!current_api.IsLegalPosition(parent)) continue;
            if (current_api.Primitive(parent) != kUndecided) continue;

            // Check if all parent positions have child as one of their
            // children.
            TierPositionArray children =
                current_api.GetCanonicalChildPositions(parent);
            if (!TierPositionArrayContains(&children, canonical_child)) {
                error = kTierSolverTestParentChildMismatchError;
            }

            TierPositionArrayDestroy(&children);
            if (error != kTierSolverTestNoError) break;
        }

        PositionArrayDestroy(&parents);
        if (error != kTierSolverTestNoError) break;
    }

    return error;
}

static int TestPrintError(Tier tier, Position position) {
    return printf("\nTierWorkerTest: error detected at position %" PRIPos
                  " of tier %" PRITier "\n",
                  position, tier);
}

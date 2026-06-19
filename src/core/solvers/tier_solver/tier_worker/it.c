/**
 * @file it.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Immediate transition tier worker algorithm implementation.
 * @version 1.1.3
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

#include "core/solvers/tier_solver/tier_worker/it.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "core/concurrency.h"
#include "core/constants.h"
#include "core/data_structures/bitset.h"
#include "core/db/db_manager.h"
#include "core/gamesman_memory.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_error.h"
#include "core/types/tier_hash_set.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

// Note on multithreading:
//   Be careful that "if (!condition) success = false;" is not equivalent to
//   "success &= condition" or "success = condition". The former creates a race
//   condition whereas the latter may overwrite an already failing result.

// Reference to the set of tier solver API functions for the current game.
static const TierSolverApi *api_internal;

static size_t mem;              // Heap memory remaining for loading tiers.
static Tier this_tier;          // The tier being solved.
static int64_t this_tier_size;  // Size of the tier being solved.

// Canonical child tiers of the tier being solved.
static TierArray canonical_child_tiers;

// ------------------------------ Step0Initialize ------------------------------

static int TierSizeComp(const void *t1, const void *t2) {
    int64_t size1 = api_internal->GetTierSize(*(const Tier *)t1);
    int64_t size2 = api_internal->GetTierSize(*(const Tier *)t2);

    return (size1 > size2) - (size1 < size2);
}

static bool Step0_0SetupChildTiers(void) {
    Tier child_tiers[kTierSolverNumChildTiersMax];
    int num_child_tiers = api_internal->GetChildTiers(this_tier, child_tiers);
    TierHashSet dedup;
    TierHashSetInit(&dedup, 0.5);
    TierArrayInit(&canonical_child_tiers);
    for (int i = 0; i < num_child_tiers; ++i) {
        Tier canonical = api_internal->GetCanonicalTier(child_tiers[i]);
        if (TierHashSetAdd(&dedup, canonical)) {
            TierArrayAppend(&canonical_child_tiers, canonical);
        }
    }

    // Sort the array of canonical child tiers in ascending size order.
    TierArraySortExplicit(&canonical_child_tiers, TierSizeComp);
    TierHashSetDestroy(&dedup);

    return true;
}

static bool Step0Initialize(const TierSolverApi *api, Tier tier,
                            size_t memlimit) {
    api_internal = api;
    mem = memlimit;
    this_tier = tier;
    this_tier_size = api_internal->GetTierSize(tier);

    // Setup the canonical child tiers array.
    if (!Step0_0SetupChildTiers()) return false;

    // Setup the solving tier.
    mem -= DbManagerTierMemUsage(this_tier, this_tier_size);
    int error = DbManagerCreateSolvingTier(this_tier, this_tier_size);
    if (error != kNoError) return false;

    if (canonical_child_tiers.size > 0) {
        // Make sure that there is enough memory to load the largest child tier.
        Tier largest_child_tier =
            canonical_child_tiers.array[canonical_child_tiers.size - 1];
        int64_t size = api_internal->GetTierSize(largest_child_tier);
        size_t largest_child_mem =
            DbManagerTierMemUsage(largest_child_tier, size);
        if (largest_child_mem > mem) return false;
    }

    return true;
}

// ------------------------------- Step1Iterate -------------------------------

static bool Step1_0LoadChildTiers(Bitset *processed) {
    // Assuming canonical_child_tiers have been sorted in ascending size order.
    for (int64_t i = canonical_child_tiers.size - 1; i >= 0; --i) {
        // Skip if already processed.
        if (BitsetTest(processed, i)) continue;

        // Check if the tier can be loaded.
        Tier child_tier = canonical_child_tiers.array[i];
        int64_t size = api_internal->GetTierSize(child_tier);
        size_t required =
            DbManagerTierMemUsage(canonical_child_tiers.array[i], size);
        if (required > mem) continue;  // Not enough memory to load this tier.

        // The tier can be loaded. Proceed to loading.
        mem -= required;
        BitsetSet(processed, i);
        int error = DbManagerLoadTier(child_tier, size);
        if (error != kNoError) return false;
    }

    return true;
}

static bool IsCanonicalPosition(Position position) {
    TierPosition tier_position = {.tier = this_tier, .position = position};
    return api_internal->GetCanonicalPosition(tier_position) == position;
}

static Value GetParentValue(Value child_value) {
    switch (child_value) {
        case kWin:
            return kLose;

        case kTie:
            return kTie;

        case kDraw:
            return kDraw;

        case kLose:
            return kWin;

        // This may happen if the weak IsLegalPosition check fails to
        // identify the parent position as illegal but correctly identifies
        // one of its children as illegal.
        case kUndecided:
            return kUndecided;
        default:
            NotReached("GetParentValue: unknown value");
    }

    return kUndecided;
}

/**
 * @brief Returns a negative value if the value-remoteness pair (v1, r1) is
 * considered a worse outcome than (v2, r2); returns a positive value if
 * better; returns 0 if the two pairs are exactly the same.
 */
static int OutcomeCompare(Value v1, int r1, Value v2, int r2) {
    static_assert(kLose < kDraw && kDraw < kTie && kTie < kWin, "");
    if (v1 != v2) return v1 - v2;

    // v1 == v2
    assert(v1 >= kLose && v1 <= kWin);

    // If the common value is kLose, return r1 - r2, otherwise return r2 - r1.
    // Reason: when losing, a larger remoteness is preferred;
    // when winning/tying, a smaller remoteness is preferred.
    return (1 - (v1 == kLose) * 2) * (r2 - r1);
}

static void FindMinOutcome(
    TierPosition positions[static kTierSolverNumChildPositionsMax],
    int num_positions, Value *min_val, int *min_remoteness) {
    // Initialize to best possible outcome: win in 0.
    *min_val = kWin;
    *min_remoteness = 0;
    for (int i = 0; i < num_positions; ++i) {
        Tier tier = positions[i].tier;
        Position pos = positions[i].position;

        // Skip this position if the tier it belongs to isn't loaded in this
        // iteration.
        if (!DbManagerIsTierLoaded(tier)) continue;

        Value value = DbManagerGetValueFromLoaded(tier, pos);
        int remoteness = DbManagerGetRemotenessFromLoaded(tier, pos);
        if (OutcomeCompare(value, remoteness, *min_val, *min_remoteness) < 0) {
            *min_val = value;
            *min_remoteness = remoteness;
        }
    }
}

static void MaximizeParent(Position parent, Value child_value,
                           int child_remoteness) {
    Value parent_value = DbManagerGetValue(parent);
    int parent_remoteness = DbManagerGetRemoteness(parent);

    Value parent_new_value = GetParentValue(child_value);
    int parent_new_remoteness = child_value == kDraw ? 0 : child_remoteness + 1;

    if (parent_value == kUndecided ||
        OutcomeCompare(parent_value, parent_remoteness, parent_new_value,
                       parent_new_remoteness) < 0) {
        // Maximize parent outcome.
        DbManagerSetValueRemoteness(parent, parent_new_value,
                                    parent_new_remoteness);
    }
}

static bool Step1_1IterateOnePass(void) {
    ConcurrentBool success;
    ConcurrentBoolInit(&success, true);
    PRAGMA_OMP(parallel for schedule(dynamic, 1024))
    for (Position pos = 0; pos < this_tier_size; ++pos) {
        if (!success) continue;  // Fail fast.
        TierPosition tier_position = {.tier = this_tier, .position = pos};

        // Skip if illegal or non-canonical.
        if (!api_internal->IsLegalPosition(tier_position) ||
            !IsCanonicalPosition(pos)) {
            continue;
        }

        Value primitive_value = api_internal->Primitive(tier_position);
        if (primitive_value != kUndecided) {  // If primitive...
            // Set value immediately and continue to the next position.
            DbManagerSetValueRemoteness(pos, primitive_value, 0);
            continue;
        }

        // tier_position is not primitive, generate child positions and minimax.
        TierPosition child_positions[kTierSolverNumChildPositionsMax];
        int num_child_positions = api_internal->GetCanonicalChildPositions(
            tier_position, child_positions);

        // Find the min child (with respect to the player at parent position.)
        Value min_child_value;
        int min_child_remoteness;
        FindMinOutcome(child_positions, num_child_positions, &min_child_value,
                       &min_child_remoteness);

        // Maximize the value of the parent position using the min child.
        MaximizeParent(pos, min_child_value, min_child_remoteness);
    }

    return ConcurrentBoolLoad(&success);
}

static void Step1_2UnloadChildTiers(void) {
    for (int64_t i = 0; i < canonical_child_tiers.size; ++i) {
        Tier child_tier = canonical_child_tiers.array[i];
        if (DbManagerIsTierLoaded(child_tier)) {
            DbManagerUnloadTier(child_tier);
            int64_t child_tier_size = api_internal->GetTierSize(child_tier);
            mem += DbManagerTierMemUsage(child_tier, child_tier_size);
        }
    }
}

static bool Step1Iterate(void) {
    bool success = false;
    Bitset *processed = BitsetCreate(canonical_child_tiers.size);
    do {
        // Load as many child tiers as possible in each iteration.
        if (!Step1_0LoadChildTiers(processed)) goto _bailout;

        // Do one pass of scanning.
        if (!Step1_1IterateOnePass()) goto _bailout;

        // Unload all child tiers.
        Step1_2UnloadChildTiers();
    } while (BitsetCount(processed) < canonical_child_tiers.size);
    success = true;

_bailout:
    Step1_2UnloadChildTiers();
    BitsetDestroy(processed);

    return success;
}

// ------------------------------- Step2FlushDb -------------------------------

static void Step2FlushDb(void) {
    if (DbManagerFlushSolvingTier(NULL) != 0) {
        fprintf(stderr,
                "Step2FlushDb: an error has occurred while flushing of the "
                "current tier. The database file for tier %" PRITier
                " may be corrupt.\n",
                this_tier);
    }
    if (DbManagerFreeSolvingTier() != 0) {
        fprintf(stderr,
                "Step2FlushDb: an error has occurred while freeing of the "
                "current tier's in-memory database. Tier: %" PRITier "\n",
                this_tier);
    }
}

// ------------------------------- Step3Cleanup -------------------------------

static void Step3Cleanup(void) {
    this_tier = kIllegalTier;
    this_tier_size = kIllegalSize;
    for (int64_t i = 0; i < canonical_child_tiers.size; ++i) {
        Tier child_tier = canonical_child_tiers.array[i];
        if (DbManagerIsTierLoaded(child_tier)) DbManagerUnloadTier(child_tier);
    }
    TierArrayDestroy(&canonical_child_tiers);
    DbManagerFreeSolvingTier();
}

// -----------------------------------------------------------------------------
// ------------------------- TierWorkerSolveItInternal -------------------------
// -----------------------------------------------------------------------------

int TierWorkerSolveITInternal(const TierSolverApi *api, Tier tier,
                              const TierSolverSolveOptions *options,
                              bool *solved) {
    if (solved != NULL) *solved = false;
    int ret = kRuntimeError;
    if (!options->force && DbManagerTierStatus(tier) == kDbTierStatusSolved) {
        goto _done;
    }

    /* Immediate transition main algorithm. */
    if (!Step0Initialize(api, tier, options->memlimit)) goto _bailout;
    if (!Step1Iterate()) goto _bailout;
    Step2FlushDb();
    if (solved != NULL) *solved = true;

_done:
    ret = kNoError;  // Success.

_bailout:
    Step3Cleanup();
    return ret;
}

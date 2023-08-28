/**
 * @file tier_manager.c
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Separated functions that
 * handle tier graph traversal and management into its own module, and
 * redesigned retrograde tier analysis process to enable concurrent solving
 * of multiple tiers.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the manager module of the Loopy Tier Solver.
 *
 * @details The tier manager module is responsible for scanning, validating, and
 * creating the tier graph in memory, keeping track of solvable and solved
 * tiers, and dispatching jobs to the tier worker module.
 *
 * @version 1.0
 * @date 2023-08-19
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

#include "core/solvers/tier_solver/tier_manager.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true, false
#include <stdint.h>    // int64_t
#include <stdio.h>     // fprintf, stderr

#include "core/gamesman_types.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/solvers/tier_solver/tier_worker.h"

typedef enum TierGraphNodeStatus {
    kStatusNotVisited,
    kStatusInProgress,
    kStatusClosed,
    kNumStatus
} TierGraphNodeStatus;

static const TierSolverApi *current_api;

// This object maps each tier to its value. The value of a tier contains
// information about its number of undecided children and discovery status.
// The discovery status is used to detect loops in the tier graph during
// the topological sort process.
static TierHashMap tier_to_value;
static TierQueue solvable_tiers;
static int64_t solved_tiers = 0;
static int64_t skipped_tiers = 0;
static int64_t failed_tiers = 0;

static bool InitGlobalVariables(void);
static void DestroyGlobalVariables(void);
static bool CreateTierTree(void);
enum CreateTierTreeProcessChildrenErrors {
    kNoError,
    kOutOfMemory,
    kLoopDetected
};
static int CreateTierTreeProcessChildren(Tier parent, TierStack *fringe);
static void EnqueuePrimitiveTiers(void);

static int64_t NumUnsolvedChildTiersAndStatusToValue(
    int num_unsolved_child_tiers, int status);
static int ValueToStatus(int64_t value);
static int ValueToNumUnsolvedChildTiers(int64_t value);

static int64_t GetValue(Tier tier);
static int GetStatus(Tier tier);
static int GetNumUnsolvedChildTiers(Tier tier);

static bool SetStatus(Tier tier, int status);
static bool SetNumUnsolvedChildTiers(Tier tier, int num_unsolved_child_tiers);

static bool IsCanonicalTier(Tier tier);

static int SolveTierTree(bool force);
static void UpdateTierTree(Tier solved_tier);
static void PrintSolverResult(void);

// -----------------------------------------------------------------------------
int TierManagerSolve(const TierSolverApi *api, bool force) {
    current_api = api;
    if (!InitGlobalVariables()) {
        fprintf(stderr, "TierManagerSolve: initialization failed.\n");
        return 1;
    }
    int ret = SolveTierTree(force);
    DestroyGlobalVariables();
    return ret;
}
// -----------------------------------------------------------------------------

static bool InitGlobalVariables(void) {
    solved_tiers = 0;
    skipped_tiers = 0;
    failed_tiers = 0;
    TierQueueInit(&solvable_tiers);
    TierHashMapInit(&tier_to_value, 0.5);
    return CreateTierTree();
}

static void DestroyGlobalVariables(void) {
    TierHashMapDestroy(&tier_to_value);
    TierQueueDestroy(&solvable_tiers);
}

/**
 * Iterative topological sort using DFS and node coloring (status marking).
 * Algorithm by Ctrl, stackoverflow.com.
 * https://stackoverflow.com/a/73210346
 */
static bool CreateTierTree(void) {
    // DFS from initial tier with loop detection.
    TierStack fringe;
    TierStackInit(&fringe);

    Tier initial_tier = current_api->GetInitialTier();
    TierStackPush(&fringe, initial_tier);
    TierHashMapSet(&tier_to_value, initial_tier,
                   NumUnsolvedChildTiersAndStatusToValue(0, kStatusNotVisited));
    while (!TierStackEmpty(&fringe)) {
        Tier parent = TierStackTop(&fringe);
        int status = GetStatus(parent);
        if (status == kStatusInProgress) {
            SetStatus(parent, kStatusClosed);
            TierStackPop(&fringe);
            continue;
        } else if (status == kStatusClosed) {
            TierStackPop(&fringe);
            continue;
        }
        SetStatus(parent, kStatusInProgress);
        int error = CreateTierTreeProcessChildren(parent, &fringe);
        switch (error) {
            case kNoError:
                continue;
                break;

            case kOutOfMemory:
                fprintf(stderr, "CreateTierTree: out of memory.\n");
                break;

            case kLoopDetected:
                fprintf(
                    stderr,
                    "CreateTierTree: a loop is detected in the tier graph.\n");
                break;
        }
        TierHashMapDestroy(&tier_to_value);
        TierStackDestroy(&fringe);
        return false;
    }
    TierStackDestroy(&fringe);
    EnqueuePrimitiveTiers();
    return true;
}

static int CreateTierTreeProcessChildren(Tier parent, TierStack *fringe) {
    TierArray tier_children = current_api->GetChildTiers(parent);
    if (!SetNumUnsolvedChildTiers(parent, (int)tier_children.size)) {
        TierArrayDestroy(&tier_children);
        return (int)kOutOfMemory;
    }
    for (int64_t i = 0; i < tier_children.size; ++i) {
        Tier child = tier_children.array[i];
        if (!TierHashMapContains(&tier_to_value, child)) {
            TierHashMapSet(
                &tier_to_value, child,
                NumUnsolvedChildTiersAndStatusToValue(0, kStatusNotVisited));
            TierStackPush(fringe, child);
            continue;
        }
        int status = GetStatus(child);
        if (status == kStatusNotVisited) {
            TierStackPush(fringe, child);
        } else if (status == kStatusInProgress) {
            TierArrayDestroy(&tier_children);
            return (int)kLoopDetected;
        }  // else, child tier is already closed and we take no action.
    }
    TierArrayDestroy(&tier_children);
    return (int)kNoError;
}

static void EnqueuePrimitiveTiers(void) {
    TierHashMapIterator it = TierHashMapBegin(&tier_to_value);
    Tier tier;
    int64_t value;
    while (TierHashMapIteratorNext(&it, &tier, &value)) {
        if (ValueToNumUnsolvedChildTiers(value) == 0) {
            TierQueuePush(&solvable_tiers, tier);
        }
    }
    // A well-formed directed acyclic tier graph should have at least one
    // primitive tier.
    assert(!TierQueueIsEmpty(&solvable_tiers));
}

static int SolveTierTree(bool force) {
    TierWorkerInit(current_api);
    while (!TierQueueIsEmpty(&solvable_tiers)) {
        Tier tier = TierQueuePop(&solvable_tiers);
        if (IsCanonicalTier(tier)) {
            // Only solve canonical tiers.
            int error = TierWorkerSolve(tier, force);
            if (error == 0) {
                // Solve succeeded.
                UpdateTierTree(tier);
                ++solved_tiers;
            } else {
                // There might be more error types in the future.
                printf("Failed to solve tier %" PRId64 ": not enough memory\n",
                       tier);
                ++failed_tiers;
            }
        } else {
            ++skipped_tiers;
        }
    }
    PrintSolverResult();
    return 0;
}

static void UpdateTierTree(Tier solved_tier) {
    TierArray parent_tiers = current_api->GetParentTiers(solved_tier);
    TierHashSet canonical_parents;
    TierHashSetInit(&canonical_parents, 0.5);
    for (int64_t i = 0; i < parent_tiers.size; ++i) {
        // Update canonical parent's number of unsolved children only.
        Tier canonical = current_api->GetCanonicalTier(parent_tiers.array[i]);
        if (TierHashSetContains(&canonical_parents, canonical)) {
            // It is possible that a child has two parents that are symmetrical
            // to each other. In this case, we should only decrement the child
            // counter once.
            continue;
        }
        TierHashSetAdd(&canonical_parents, canonical);
        int num_unsolved_child_tiers = GetNumUnsolvedChildTiers(canonical);
        assert(num_unsolved_child_tiers > 0);
        bool success =
            SetNumUnsolvedChildTiers(canonical, num_unsolved_child_tiers - 1);
        assert(success);
        (void)success;
        if (num_unsolved_child_tiers == 1)
            TierQueuePush(&solvable_tiers, canonical);
    }
    TierHashSetDestroy(&canonical_parents);
    TierArrayDestroy(&parent_tiers);
}

static int64_t NumUnsolvedChildTiersAndStatusToValue(
    int num_unsolved_child_tiers, int status) {
    return num_unsolved_child_tiers * kNumStatus + status;
}

static int ValueToStatus(int64_t value) { return value % kNumStatus; }

static int ValueToNumUnsolvedChildTiers(int64_t value) {
    return value / kNumStatus;
}

static int64_t GetValue(Tier tier) {
    TierHashMapIterator it = TierHashMapGet(&tier_to_value, tier);
    assert(TierHashMapIteratorIsValid(&it));
    return TierHashMapIteratorValue(&it);
}

static int GetStatus(Tier tier) { return ValueToStatus(GetValue(tier)); }

static int GetNumUnsolvedChildTiers(Tier tier) {
    return ValueToNumUnsolvedChildTiers(GetValue(tier));
}

static bool SetStatus(Tier tier, int status) {
    int64_t value = GetValue(tier);
    int num_unsolved_child_tiers = ValueToNumUnsolvedChildTiers(value);
    value =
        NumUnsolvedChildTiersAndStatusToValue(num_unsolved_child_tiers, status);
    return TierHashMapSet(&tier_to_value, tier, value);
}

static bool SetNumUnsolvedChildTiers(Tier tier, int num_unsolved_child_tiers) {
    int64_t value = GetValue(tier);
    int status = ValueToStatus(value);
    value =
        NumUnsolvedChildTiersAndStatusToValue(num_unsolved_child_tiers, status);
    return TierHashMapSet(&tier_to_value, tier, value);
}

static bool IsCanonicalTier(Tier tier) {
    return current_api->GetCanonicalTier(tier) == tier;
}

static void PrintSolverResult(void) {
    printf(
        "Finished solving all tiers.\n"
        "Number of canonical tiers solved: %" PRId64
        "\n"
        "Number of non-canonical tiers skipped: %" PRId64
        "\n"
        "Number of tiers failed due to OOM: %" PRId64
        "\n"
        "Total tiers scanned: %" PRId64 "\n",
        solved_tiers, skipped_tiers, failed_tiers,
        solved_tiers + skipped_tiers + failed_tiers);
    printf("\n");
}

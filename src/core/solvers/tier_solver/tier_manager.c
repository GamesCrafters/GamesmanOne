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

#include "core/analysis/analysis.h"
#include "core/gamesman_types.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_analyzer.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/solvers/tier_solver/tier_worker.h"

enum TierManagementType {
    kTierSolving,
    kTierDiscovering,
};

typedef enum TierGraphNodeStatus {
    kStatusNotVisited,
    kStatusInProgress,
    kStatusClosed,
    kNumStatus
} TierGraphNodeStatus;

enum TierTreeErrorTypes {
    kNoError,
    kOutOfMemory,
    kLoopDetected,
};

static const TierSolverApi *current_api;

// The tier tree that maps each tier to its value. The value of a tier contains
// information about its number of undecided children (or undiscovered parents
// if the tree is reversed) and discovery status. The discovery status is used
// to detect loops in the tier graph during topological sort.
static TierHashMap tier_tree;
static TierQueue pending_tiers;
static int64_t processed_tiers = 0;
static int64_t skipped_tiers = 0;
static int64_t failed_tiers = 0;
static Analysis game_analysis;

static int InitGlobalVariables(int type);
static void DestroyGlobalVariables(void);

static int BuildTierTree(int type);
static int BuildTierTreeProcessChildren(Tier parent, TierStack *fringe,
                                        int type);
static int EnqueuePrimitiveTiers(void);
static void CreateTierTreePrintError(int error);

static int64_t NumTiersAndStatusToValue(int num_tiers, int status);
static int ValueToStatus(int64_t value);
static int ValueToNumTiers(int64_t value);

static int64_t GetValue(Tier tier);
static int GetStatus(Tier tier);
static int GetNumTiers(Tier tier);

static bool TierTreeSetInitial(Tier tier);
static bool TierTreeSetStatus(Tier tier, int status);
static bool TierTreeSetNumTiers(Tier tier, int num_tiers);
static bool IncrementNumParentTiers(Tier tier);

static bool IsCanonicalTier(Tier tier);

static int SolveTierTree(bool force);
static void SolveUpdateTierTree(Tier solved_tier);
static void PrintSolverResult(void);

static int DiscoverTierTree(bool force);
static void AnalyzeUpdateTierTree(Tier analyzed_tier);
static void PrintAnalyzerResult(void);

// -----------------------------------------------------------------------------

int TierManagerSolve(const TierSolverApi *api, bool force) {
    current_api = api;
    if (InitGlobalVariables(kTierSolving) != 0) {
        fprintf(stderr, "TierManagerSolve: initialization failed.\n");
        return 1;
    }
    int ret = SolveTierTree(force);
    DestroyGlobalVariables();
    return ret;
}

int TierManagerAnalyze(const TierSolverApi *api, bool force) {
    current_api = api;
    if (InitGlobalVariables(kTierDiscovering) != 0) {
        fprintf(stderr, "TierManagerAnalyze: initialization failed.\n");
        return 1;
    }
    int ret = DiscoverTierTree(force);
    DestroyGlobalVariables();
    return ret;
}

// -----------------------------------------------------------------------------

static int InitGlobalVariables(int type) {
    processed_tiers = 0;
    skipped_tiers = 0;
    failed_tiers = 0;
    TierQueueInit(&pending_tiers);
    TierHashMapInit(&tier_tree, 0.5);
    if (type == kTierDiscovering) {
        AnalysisInit(&game_analysis);
        AnalysisSetHashSize(&game_analysis, 0);
    }
    return BuildTierTree(type);
}

static void DestroyGlobalVariables(void) {
    TierHashMapDestroy(&tier_tree);
    TierQueueDestroy(&pending_tiers);
    AnalysisDestroy(&game_analysis);
}

/**
 * @brief DFS from initial tier with loop detection.
 *
 * @details Iterative topological sort using DFS and node coloring (status
 * marking). Algorithm by Ctrl, stackoverflow.com.
 * @link https://stackoverflow.com/a/73210346
 */
static int BuildTierTree(int type) {
    int ret = 0;
    TierStack fringe;
    TierStackInit(&fringe);

    Tier initial_tier = current_api->GetInitialTier();
    if (!TierStackPush(&fringe, initial_tier)) {
        ret = 1;
        goto _bailout;
    }

    if (!TierTreeSetInitial(initial_tier)) {
        ret = 1;
        goto _bailout;
    }

    while (!TierStackEmpty(&fringe)) {
        Tier parent = TierStackTop(&fringe);
        int status = GetStatus(parent);
        if (status == kStatusInProgress) {
            if (!TierTreeSetStatus(parent, kStatusClosed)) {
                ret = 1;
                goto _bailout;
            }
            TierStackPop(&fringe);
            continue;
        } else if (status == kStatusClosed) {
            TierStackPop(&fringe);
            continue;
        }
        TierTreeSetStatus(parent, kStatusInProgress);
        int error = BuildTierTreeProcessChildren(parent, &fringe, type);
        switch (error) {
            case kNoError:
                continue;
                break;

            case kOutOfMemory:
                ret = 1;
                goto _bailout;
                break;

            case kLoopDetected:
                ret = 2;
                goto _bailout;
                break;
        }
    }

_bailout:
    TierStackDestroy(&fringe);
    if (ret != 0) {
        TierHashMapDestroy(&tier_tree);
        CreateTierTreePrintError(ret);
    } else if (type == kTierSolving) {
        EnqueuePrimitiveTiers();
    } else {
        TierQueuePush(&pending_tiers, initial_tier);
    }
    return ret;
}

static int BuildTierTreeProcessChildren(Tier parent, TierStack *fringe,
                                        int type) {
    TierArray tier_children = current_api->GetChildTiers(parent);
    if (type == kTierSolving) {
        if (!TierTreeSetNumTiers(parent, (int)tier_children.size)) {
            TierArrayDestroy(&tier_children);
            return (int)kOutOfMemory;
        }
    } else {
        for (int64_t i = 0; i < tier_children.size; ++i) {
            Tier child = tier_children.array[i];
            if (!IncrementNumParentTiers(child)) {
                TierArrayDestroy(&tier_children);
                return 1;
            }
        }
    }

    for (int64_t i = 0; i < tier_children.size; ++i) {
        Tier child = tier_children.array[i];
        if (!TierHashMapContains(&tier_tree, child)) {
            if (!TierTreeSetInitial(child)) {
                fprintf(stderr,
                        "BuildTierTreeProcessChildren: failed to set new child "
                        "in tier tree.\n");
                TierArrayDestroy(&tier_children);
                return (int)kOutOfMemory;
            }
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

static int EnqueuePrimitiveTiers(void) {
    TierHashMapIterator it = TierHashMapBegin(&tier_tree);
    Tier tier;
    int64_t value;
    while (TierHashMapIteratorNext(&it, &tier, &value)) {
        if (ValueToNumTiers(value) == 0) {
            if (!TierQueuePush(&pending_tiers, tier)) return 1;
        }
    }

    if (TierQueueIsEmpty(&pending_tiers)) {
        fprintf(stderr,
                "EnqueuePrimitiveTiers: (BUG) The tier graph contains no "
                "primitive tiers.\n");
        return 2;
    }
    return 0;
}

static void CreateTierTreePrintError(int error) {
    switch (error) {
        case kNoError:
            break;

        case kOutOfMemory:
            fprintf(stderr, "BuildTierTree: out of memory.\n");
            break;

        case kLoopDetected:
            fprintf(stderr,
                    "BuildTierTree: a loop is detected in the tier graph.\n");
            break;
    }
}

static int SolveTierTree(bool force) {
    TierWorkerInit(current_api);
    while (!TierQueueIsEmpty(&pending_tiers)) {
        Tier tier = TierQueuePop(&pending_tiers);
        if (IsCanonicalTier(tier)) {
            // Only solve canonical tiers.
            int error = TierWorkerSolve(tier, force);
            if (error == 0) {
                // Solve succeeded.
                SolveUpdateTierTree(tier);
                ++processed_tiers;
            } else {
                printf("Failed to solve tier %" PRId64 ", code %d\n", tier,
                       error);
                ++failed_tiers;
            }
        } else {
            ++skipped_tiers;
        }
    }
    PrintSolverResult();
    return 0;
}

static void SolveUpdateTierTree(Tier solved_tier) {
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
        int num_unsolved_child_tiers = GetNumTiers(canonical);
        assert(num_unsolved_child_tiers > 0);
        bool success =
            TierTreeSetNumTiers(canonical, num_unsolved_child_tiers - 1);
        assert(success);
        (void)success;
        if (num_unsolved_child_tiers == 1)
            TierQueuePush(&pending_tiers, canonical);
    }
    TierHashSetDestroy(&canonical_parents);
    TierArrayDestroy(&parent_tiers);
}

static int64_t NumTiersAndStatusToValue(int num_tiers, int status) {
    return num_tiers * kNumStatus + status;
}

static int ValueToStatus(int64_t value) { return value % kNumStatus; }

static int ValueToNumTiers(int64_t value) { return value / kNumStatus; }

static int64_t GetValue(Tier tier) {
    TierHashMapIterator it = TierHashMapGet(&tier_tree, tier);
    if (!TierHashMapIteratorIsValid(&it)) return -1;
    return TierHashMapIteratorValue(&it);
}

static int GetStatus(Tier tier) { return ValueToStatus(GetValue(tier)); }

static int GetNumTiers(Tier tier) { return ValueToNumTiers(GetValue(tier)); }

static bool TierTreeSetInitial(Tier tier) {
    assert(!TierHashMapContains(&tier_tree, tier));
    int64_t value = NumTiersAndStatusToValue(0, kStatusNotVisited);
    return TierHashMapSet(&tier_tree, tier, value);
}

static bool TierTreeSetStatus(Tier tier, int status) {
    int64_t value = GetValue(tier);
    int num_tiers = ValueToNumTiers(value);
    value = NumTiersAndStatusToValue(num_tiers, status);
    return TierHashMapSet(&tier_tree, tier, value);
}

static bool TierTreeSetNumTiers(Tier tier, int num_tiers) {
    int64_t value = GetValue(tier);
    int status = ValueToStatus(value);
    value = NumTiersAndStatusToValue(num_tiers, status);
    return TierHashMapSet(&tier_tree, tier, value);
}

static bool IncrementNumParentTiers(Tier tier) {
    int64_t value = GetValue(tier);
    if (value < 0) value = NumTiersAndStatusToValue(0, kStatusNotVisited);

    int status = ValueToStatus(value);
    int num_tiers = ValueToNumTiers(value) + 1;
    value = NumTiersAndStatusToValue(num_tiers, status);
    return TierHashMapSet(&tier_tree, tier, value);
}

static bool IsCanonicalTier(Tier tier) {
    return current_api->GetCanonicalTier(tier) == tier;
}

static void PrintSolverResult(void) {
    printf(
        "Finished solving all tiers.\n"
        "Number of canonical tiers solved: %" PRId64
        "\nNumber of non-canonical tiers skipped: %" PRId64
        "\nNumber of tiers failed due to OOM: %" PRId64
        "\nTotal tiers scanned: %" PRId64 "\n",
        processed_tiers, skipped_tiers, failed_tiers,
        processed_tiers + skipped_tiers + failed_tiers);
    printf("\n");
}

static int DiscoverTierTree(bool force) {
    TierAnalyzerInit(current_api);
    while (!TierQueueIsEmpty(&pending_tiers)) {
        Tier tier = TierQueuePop(&pending_tiers);
        Tier canonical = current_api->GetCanonicalTier(tier);

        // Analyze the canonical tier instead.
        Analysis tier_analysis;
        int error = TierAnalyzerDiscover(&tier_analysis, canonical, force);
        if (error == 0) {
            // Analyzer succeeded.
            AnalyzeUpdateTierTree(tier);
            ++processed_tiers;
        } else {
            printf("Failed to analyze tier %" PRId64 ", code %d\n", tier,
                   error);
            ++failed_tiers;
        }

        if (tier != canonical) {
            // TODO: Change Aggregate instead of using this conversion!
            AnalysisConvertToNoncanonical(&tier_analysis);
        }

        AnalysisAggregate(&game_analysis, &tier_analysis);
        AnalysisDestroy(&tier_analysis);
    }

    PrintAnalyzerResult();
    AnalysisPrintEverything(stdout, &game_analysis);
    return 0;
}

static void AnalyzeUpdateTierTree(Tier analyzed_tier) {
    TierArray child_tiers = current_api->GetChildTiers(analyzed_tier);
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        Tier child = child_tiers.array[i];
        int num_undiscovered_parent_tiers = GetNumTiers(child);
        assert(num_undiscovered_parent_tiers > 0);
        if (!TierTreeSetNumTiers(child, num_undiscovered_parent_tiers - 1)) {
            NotReached(
                "AnalyzeUpdateTierTree: unexpected error while resetting an "
                "existing entry in tier hash map");
        }
        if (num_undiscovered_parent_tiers == 1) {
            TierQueuePush(&pending_tiers, child);
        }
    }
    TierArrayDestroy(&child_tiers);
}

static void PrintAnalyzerResult(void) {
    printf(
        "Finished analyzing all tiers.\n"
        "Number of canonical tiers analyzed: %" PRId64
        "\nNumber of tiers failed due to OOM: %" PRId64
        "\nTotal tiers scanned: %" PRId64 "\n",
        processed_tiers, failed_tiers, processed_tiers + failed_tiers);
    printf("\n");
}

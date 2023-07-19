#include "core/solver.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRId64
#include <malloc.h>    // free
#include <stdbool.h>   // bool, true, false
#include <stdint.h>    // int8_t, int64_t
#include <stdio.h>     // printf
#include <string.h>    // memset

#include "core/gamesman.h"
#include "core/gamesman_types.h"
#include "core/misc.h"
#include "core/naivedb.h"
#include "core/tier_solver/tier_solver.h"

enum TierGraphNodeStatus {
    kStatusNotVisited,
    kStatusInProgress,
    kStatusClosed,
    kNumStatus
};

static TierHashMap map;
static TierQueue solvable_tiers;
static int64_t solved_tiers = 0;
static int64_t skipped_tiers = 0;
static int64_t failed_tiers = 0;

static bool SelectApiFunctions(void);
static bool SelectTierApi(void);
static bool SelectRegularApi(void);

static bool InitGlobalVariables(void);
static void DestroyGlobalVariables(void);
static bool CreateTierTree(void);
static bool CreateTierTreeProcessChildren(Tier parent, TierStack *fringe);
static void EnqueuePrimitiveTiers(void);
static int64_t CalcVal(int num_unsolved_child_tiers, int status);
static int CalcStatus(int64_t value);
static int CalcNumUnsolvedChildTiers(int64_t value);
static int64_t GetValue(Tier tier);
static int GetStatus(Tier tier);
static int GetNumUnsolvedChildTiers(Tier tier);
static bool SetStatus(Tier tier, int status);
static bool SetNumUnsolvedChildTiers(Tier tier, int num_unsolved_child_tiers);

static Value SolveTierTree(bool force);
static void UpdateTierTree(Tier solved_tie);
static void PrintSolverResult(void);

// ----------------------------------------------------------------------------
Value SolverSolve(bool force) {
    if (!SelectApiFunctions()) {
        fprintf(stderr,
                "failed to set up solver due to missing required API.\n");
        return kUndecided;
    }
    if (!InitGlobalVariables()) {
        fprintf(stderr,
                "initialization failed because there is a loop in the tier "
                "graph.\n");
        return kUndecided;
    }
    Value ret = SolveTierTree(force);
    DestroyGlobalVariables();
    return ret;
}
// ----------------------------------------------------------------------------

static bool SelectApiFunctions(void) {
    if (global_num_positions == kDefaultGlobalNumberOfPositions) {
        // Game initialization function failed to set global_num_positions.
        return false;
    } else if (global_num_positions == kTierGamesmanGlobalNumberOfPositions) {
        return SelectTierApi();
    } else {
        return SelectRegularApi();
    }
}

static bool SelectTierApi(void) {
    // TODO
    return false;
}

static bool SelectRegularApi(void) {
    // Check for required API.
    assert(global_num_positions > 0);
    if (global_initial_position < 0) return false;
    if (GenerateMoves == NULL || Primitive == NULL || DoMove == NULL)
        return false;

    // Generate optional regular API if needed.
    if (GetCanonicalPosition == NULL)
        GetCanonicalPosition = &GamesmanGetCanonicalPosition;
    if (GetNumberOfCanonicalChildPositions == NULL)
        GetNumberOfCanonicalChildPositions =
            &GamesmanGetNumberOfCanonicalChildPositions;
    if (GetCanonicalChildPositions == NULL)
        GetCanonicalChildPositions = &GamesmanGetCanonicalChildPositions;

    // Convert regular API to tier API.
    global_initial_tier = 0;
    TierGenerateMoves = &GamesmanTierGenerateMovesConverted;
    TierPrimitive = &GamesmanTierPrimitiveConverted;
    TierDoMove = &GamesmanTierDoMoveConverted;
    TierIsLegalPosition = &GamesmanTierIsLegalPositionConverted;
    TierGetCanonicalPosition = &GamesmanTierGetCanonicalPositionConverted;

    // Tier position API.
    TierGetNumberOfCanonicalChildPositions =
        &GamesmanTierGetNumberOfCanonicalChildPositionsConverted;
    GetTierSize = &GamesmanGetTierSizeConverted;
    if (GetCanonicalParentPositions) {
        TierGetCanonicalParentPositions =
            &GamesmanTierGetCanonicalParentPositionsConverted;
    } else {
        // TODO: Otherwise, build backward graph in memory.
        return false;
    }

    // Tier tree API.
    GetChildTiers = &GamesmanGetChildTiersConverted;
    GetParentTiers = &GamesmanGetParentTiersConverted;
    IsCanonicalTier = &GamesmanIsCanonicalTierConverted;
    GetCanonicalTier = &GamesmanGetCanonicalTierConverted;
    return true;
}

static bool InitGlobalVariables(void) {
    solved_tiers = 0;
    skipped_tiers = 0;
    failed_tiers = 0;
    TierQueueInit(&solvable_tiers);
    TierHashMapInit(&map, 0.5);
    return CreateTierTree();
}

static void DestroyGlobalVariables(void) {
    TierHashMapDestroy(&map);
    TierQueueDestroy(&solvable_tiers);
}

/**
 * Iterative topological sort using DFS and node coloring.
 * Algorithm by Ctrl, stackoverflow.com.
 * https://stackoverflow.com/a/73210346
 */
static bool CreateTierTree(void) {
    // DFS from initial tier with loop detection.
    TierStack fringe;
    TierStackInit(&fringe);
    TierStackPush(&fringe, global_initial_tier);
    TierHashMapSet(&map, global_initial_tier, CalcVal(0, kStatusNotVisited));
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
        if (!CreateTierTreeProcessChildren(parent, &fringe)) {
            // Consider printing debug info here.
            TierHashMapDestroy(&map);
            TierStackDestroy(&fringe);
            return false;
        }
    }
    TierStackDestroy(&fringe);
    EnqueuePrimitiveTiers();
    return true;
}

static bool CreateTierTreeProcessChildren(Tier parent, TierStack *fringe) {
    TierArray tier_children = GetChildTiers(parent);
    if (!SetNumUnsolvedChildTiers(parent, (int)tier_children.size)) {
        TierArrayDestroy(&tier_children);
        return false;
    }
    for (int64_t i = 0; i < tier_children.size; ++i) {
        Tier child = tier_children.array[i];
        if (!TierHashMapContains(&map, child)) {
            TierHashMapSet(&map, child, CalcVal(0, kStatusNotVisited));
            TierStackPush(fringe, child);
            continue;
        }
        int status = GetStatus(child);
        if (status == kStatusNotVisited) {
            TierStackPush(fringe, child);
        } else if (status == kStatusInProgress) {
            TierArrayDestroy(&tier_children);
            return false;
        }  // else, child tier is already closed and we take no action.
    }
    TierArrayDestroy(&tier_children);
    return true;
}

static void EnqueuePrimitiveTiers(void) {
    TierHashMapIterator it;
    TierHashMapIteratorInit(&it, &map);
    Tier tier;
    int64_t value;
    while (TierHashMapIteratorNext(&it, &tier, &value)) {
        if (CalcNumUnsolvedChildTiers(value) == 0) {
            TierQueuePush(&solvable_tiers, tier);
        }
    }
    // A well-formed tier DAG should have at least one primitive tier.
    assert(!TierQueueIsEmpty(&solvable_tiers));
}

static Value SolveTierTree(bool force) {
    while (!TierQueueIsEmpty(&solvable_tiers)) {
        Tier tier = TierQueuePop(&solvable_tiers);
        if (IsCanonicalTier(tier)) {
            // Only solve canonical tiers.
            int error = TierSolverSolve(tier, force);
            if (error == 0) {
                // Solve succeeded.
                UpdateTierTree(tier);
                // UpdateGlobalStatistics(stat);
                // PrintStatistics(tier, stat);
                DbDumpTierAnalysisToGlobal();
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
    AnalysisPrintSummary(&global_analysis);

    // TODO: link prober and return value of initial position if possible.
    return kUndecided;
}

static void UpdateTierTree(Tier solved_tier) {
    TierArray parent_tiers = GetParentTiers(solved_tier);
    TierHashSet canonical_parents;
    TierHashSetInit(&canonical_parents, 0.5);
    for (int64_t i = 0; i < parent_tiers.size; ++i) {
        // Update canonical parent's number of unsolved children only.
        Tier canonical = GetCanonicalTier(parent_tiers.array[i]);
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
        if (num_unsolved_child_tiers == 1)
            TierQueuePush(&solvable_tiers, canonical);
    }
    TierHashSetDestroy(&canonical_parents);
    TierArrayDestroy(&parent_tiers);
}

static int64_t CalcVal(int num_unsolved_child_tiers, int status) {
    return num_unsolved_child_tiers * kNumStatus + status;
}

static int CalcStatus(int64_t value) { return value % kNumStatus; }

static int CalcNumUnsolvedChildTiers(int64_t value) {
    return value / kNumStatus;
}

static int64_t GetValue(Tier tier) {
    TierHashMapIterator it = TierHashMapGet(&map, tier);
    assert(TierHashMapIteratorIsValid(&it));
    return TierHashMapIteratorValue(&it);
}

static int GetStatus(Tier tier) { return CalcStatus(GetValue(tier)); }

static int GetNumUnsolvedChildTiers(Tier tier) {
    return CalcNumUnsolvedChildTiers(GetValue(tier));
}

static bool SetStatus(Tier tier, int status) {
    int64_t value = GetValue(tier);
    int num_unsolved_child_tiers = CalcNumUnsolvedChildTiers(value);
    value = CalcVal(num_unsolved_child_tiers, status);
    return TierHashMapSet(&map, tier, value);
}

static bool SetNumUnsolvedChildTiers(Tier tier, int num_unsolved_child_tiers) {
    int64_t value = GetValue(tier);
    int status = CalcStatus(value);
    value = CalcVal(num_unsolved_child_tiers, status);
    return TierHashMapSet(&map, tier, value);
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

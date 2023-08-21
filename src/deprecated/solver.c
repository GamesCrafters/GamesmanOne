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

static Value SolveTierTree(bool force);
static void UpdateTierTree(Tier solved_tier);
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
    // Check for required API.
    if (global_initial_tier < 0) return false;
    if (global_initial_position < 0) return false;
    if (tier_solver.GetTierSize == NULL) return false;
    if (tier_solver.GenerateMoves == NULL) return false;
    if (tier_solver.Primitive == NULL) return false;
    if (tier_solver.DoMove == NULL) return false;
    if (tier_solver.IsLegalPosition == NULL) return false;
    if (tier_solver.GetChildTiers == NULL) return false;
    if (tier_solver.GetParentTiers == NULL) return false;

    // Turn off Tier Symmetry Removal if all required functions are not set.
    if (tier_solver.GetCanonicalTier == NULL ||
        tier_solver.GetPositionInNonCanonicalTier == NULL) {
        tier_solver.GetCanonicalTier = &GamesmanGetCanonicalTierDefault;
        tier_solver.GetPositionInNonCanonicalTier = NULL;
    }

    // Turn off Position Symmetry Removal if GetCanonicalPosition is not set.
    if (tier_solver.GetCanonicalPosition == NULL) {
        tier_solver.GetCanonicalPosition =
            &GamesmanTierGetCanonicalPositionDefault;
    }

    if (tier_solver.GetNumberOfCanonicalChildPositions == NULL) {
        tier_solver.GetNumberOfCanonicalChildPositions =
            GamesmanTierGetNumberOfCanonicalChildPositionsDefault;
    }
    if (tier_solver.GetCanonicalChildPositions == NULL) {
        tier_solver.GetCanonicalChildPositions =
            GamesmanTierGetCanonicalChildPositionsDefault;
    }
    return true;
}

static bool SelectRegularApi(void) {
    // Check for required API.
    assert(global_num_positions > 0);
    if (global_initial_position < 0) return false;
    if (regular_solver.GenerateMoves == NULL) return false;
    if (regular_solver.Primitive == NULL) return false;
    if (regular_solver.DoMove == NULL) return false;
    if (regular_solver.IsLegalPosition == NULL) return false;

    // Generate optional regular API if needed.
    if (regular_solver.GetCanonicalPosition == NULL)
        regular_solver.GetCanonicalPosition = &GamesmanGetCanonicalPosition;
    if (regular_solver.GetNumberOfCanonicalChildPositions == NULL) {
        regular_solver.GetNumberOfCanonicalChildPositions =
            &GamesmanGetNumberOfCanonicalChildPositions;
    }
    if (regular_solver.GetCanonicalChildPositions == NULL) {
        regular_solver.GetCanonicalChildPositions =
            &GamesmanGetCanonicalChildPositions;
    }

    // Convert regular API to tier API.
    global_initial_tier = 0;
    tier_solver.GetTierSize = &GamesmanGetTierSizeConverted;
    tier_solver.GenerateMoves = &GamesmanTierGenerateMovesConverted;
    tier_solver.Primitive = &GamesmanTierPrimitiveConverted;
    tier_solver.DoMove = &GamesmanTierDoMoveConverted;
    tier_solver.IsLegalPosition = &GamesmanTierIsLegalPositionConverted;
    tier_solver.GetNumberOfCanonicalChildPositions =
        &GamesmanTierGetNumberOfCanonicalChildPositionsConverted;
    tier_solver.GetCanonicalChildPositions =
        &GamesmanTierGetCanonicalChildPositionsConverted;
    tier_solver.GetCanonicalPosition =
        &GamesmanTierGetCanonicalPositionConverted;
    if (regular_solver.GetCanonicalParentPositions != NULL) {
        tier_solver.GetCanonicalParentPositions =
            &GamesmanTierGetCanonicalParentPositionsConverted;
    }

    // Tier tree API.
    tier_solver.GetChildTiers = &GamesmanGetChildTiersConverted;
    tier_solver.GetParentTiers = &GamesmanGetParentTiersConverted;
    tier_solver.GetCanonicalTier = &GamesmanGetCanonicalTierDefault;
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
    TierHashMapSet(&map, global_initial_tier,
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
    TierArray tier_children = tier_solver.GetChildTiers(parent);
    if (!SetNumUnsolvedChildTiers(parent, (int)tier_children.size)) {
        TierArrayDestroy(&tier_children);
        return false;
    }
    for (int64_t i = 0; i < tier_children.size; ++i) {
        Tier child = tier_children.array[i];
        if (!TierHashMapContains(&map, child)) {
            TierHashMapSet(
                &map, child,
                NumUnsolvedChildTiersAndStatusToValue(0, kStatusNotVisited));
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
        if (ValueToNumUnsolvedChildTiers(value) == 0) {
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
    TierArray parent_tiers = tier_solver.GetParentTiers(solved_tier);
    TierHashSet canonical_parents;
    TierHashSetInit(&canonical_parents, 0.5);
    for (int64_t i = 0; i < parent_tiers.size; ++i) {
        // Update canonical parent's number of unsolved children only.
        Tier canonical = tier_solver.GetCanonicalTier(parent_tiers.array[i]);
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
    TierHashMapIterator it = TierHashMapGet(&map, tier);
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
    return TierHashMapSet(&map, tier, value);
}

static bool SetNumUnsolvedChildTiers(Tier tier, int num_unsolved_child_tiers) {
    int64_t value = GetValue(tier);
    int status = ValueToStatus(value);
    value =
        NumUnsolvedChildTiersAndStatusToValue(num_unsolved_child_tiers, status);
    return TierHashMapSet(&map, tier, value);
}

static bool IsCanonicalTier(Tier tier) {
    return tier_solver.GetCanonicalTier(tier) == tier;
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
#include "solver.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRId64
#include <malloc.h>    // free
#include <stdbool.h>   // bool, true, false
#include <stdint.h>    // int8_t, int64_t
#include <stdio.h>     // printf
#include <string.h>    // memset

#include "gamesman.h"
#include "gamesman_types.h"
#include "misc.h"
#include "tier_hash_table.h"
#include "tier_solver.h"

static const int8_t kStatusNotVisited = 0;
static const int8_t kStatusInProgress = 1;
static const int8_t kStatusClosed = 2;

static TierHashTable table;
static TierHashTableEntryList solvable_tiers;
static TierSolverStatistics global_stat;
static int64_t solved_tiers = 0;
static int64_t skipped_tiers = 0;
static int64_t failed_tiers = 0;

static bool SelectApiFunctions(void);
static bool SelectTierApi(void);
static bool SelectRegularApi(void);

static bool InitializeGlobalVariables(void);
static void DestroyGlobalVariables(void);
static void CreateSolverTable(void);
static bool CreateSolverTableProcessChildren(Tier parent, TierStack *fringe);
static void SeparatePrimitiveTiers(void);

static Value SolveTierTree(void);
static int8_t *TierHashTableEntryGetStatus(TierHashTableEntry *entry);
static int32_t *TierHashTableEntryGetNumUnsolvedChildren(
    TierHashTableEntry *entry);
static TierHashTableEntry *GetTierHashTableEntryListTail(
    TierHashTableEntryList list);
static void UpdateTierHashTable(Tier solved_tier,
                                TierHashTableEntry **solvable_tail);
static void UpdateGlobalStatistics(TierSolverStatistics stat);
static void PrintStatistics(Tier solved_tier, TierSolverStatistics stat);
static void PrintSolverResult(void);

// ----------------------------------------------------------------------------
Value SolverSolve(void) {
    if (!SelectApiFunctions()) {
        fprintf(stderr,
                "failed to set up solver due to missing required API.\n");
        return kUndecided;
    }
    if (!InitializeGlobalVariables()) {
        fprintf(stderr,
                "initialization failed because there is a loop in the tier "
                "graph.\n");
        return kUndecided;
    }
    Value ret = SolveTierTree();
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
    // TODO: Double check this function!
    if (!GenerateMoves || !Primitive || !DoMove) return false;
    if (global_initial_position < 0) return false;
    TierGenerateMoves = &GamesmanTierGenerateMoves;
    TierPrimitive = &GamesmanTierPrimitive;
    TierDoMove = &GamesmanTierDoMove;
    if (!GetNumberOfChildPositions)
        GetNumberOfChildPositions = &GamesmanGetNumberOfChildPositions;
    if (!GetChildPositions) GetChildPositions = &GamesmanGetChildPositions;

    TierGetNumberOfChildPositions = &GamesmanTierGetNumberOfChildPositions;
    GetTierSize = &GamesmanGetTierSize;
    if (GetParentPositions) {
        TierGetParentPositions = &GamesmanTierGetParentPositions;
    } else {
        // TODO: Otherwise, build backward graph in memory.
        return false;
    }
    return true;
}

static bool InitializeGlobalVariables(void) {
    memset(&global_stat, 0, sizeof(global_stat));
    solved_tiers = 0;
    skipped_tiers = 0;
    failed_tiers = 0;
    solvable_tiers = NULL;
    if (!TierHashTableInitialize(&table)) return false;
    CreateSolverTable();  // Also sets solvable_tiers if successful.
    // Otherwise, solvable_tiers is not set.
    return (solvable_tiers != NULL);
}

static void DestroyGlobalVariables(void) {
    TierHashTableDestroy(&table);
    while (solvable_tiers) {
        TierHashTableEntry *next = solvable_tiers->next;
        free(solvable_tiers);
        solvable_tiers = next;
    }
}

/**
 * Algorithm by Ctrl, stackoverflow.com.
 * https://stackoverflow.com/a/73210346
 */
static void CreateSolverTable(void) {
    // DFS from initial tier with loop detection.
    TierStack fringe;
    TierStackInit(&fringe);
    TierStackPush(&fringe, global_initial_tier);
    TierHashTableAdd(global_initial_position);
    while (!TierStackEmpty(&fringe)) {
        Tier parent = TierStackTop(&fringe);
        TierHashTableEntry *entry = TierHashTableFind(&table, parent);
        assert(entry);
        if (*TierHashTableEntryGetStatus(entry) == kStatusInProgress) {
            *TierHashTableEntryGetStatus(entry) = kStatusClosed;
            TierStackPop(&fringe);
            continue;
        } else if (*TierHashTableEntryGetStatus(entry) == kStatusClosed) {
            TierStackPop(&fringe);
            continue;
        }
        *TierHashTableEntryGetStatus(entry) = kStatusInProgress;
        if (!CreateSolverTableProcessChildren(parent, &fringe)) {
            TierHashTableClear(&table);
            TierStackDestroy(&fringe);
            return;
        }
    }
    TierStackDestroy(&fringe);
    SeparatePrimitiveTiers();
}

static bool CreateSolverTableProcessChildren(Tier parent, TierStack *fringe) {
    TierArray tier_children = GetChildTiers(parent);
    for (int64_t i = 0; i < tier_children.size; ++i) {
        Tier child = tier_children.array[i];
        TierHashTableEntry *child_entry = TierHashTableFind(&table, child);
        if (child_entry == NULL) {
            TierHashTableAdd(child);
            TierStackPush(fringe, child);
        } else if (*TierHashTableEntryGetStatus(child_entry) ==
                   kStatusNotVisited) {
            TierStackPush(fringe, child);
        } else if (*TierHashTableEntryGetStatus(child_entry) ==
                   kStatusInProgress) {
            TierArrayDestroy(&tier_children);
            return false;
        }  // else, child tier is already closed and we take no action.
    }
    TierArrayDestroy(&tier_children);
    return true;
}

static void SeparatePrimitiveTiers(void) {
    TierHashTableIterator it;
    bool success = TierHashTableIteratorInit(&it, &table);
    assert(success);
    for (Tier current = TierHashTableIteratorGetNext(&it); current >= 0;
         current = TierHashTableIteratorGetNext(&it)) {
        TierHashTableEntry *entry = TierHashTableRemove(&table, current);
        entry->next = solvable_tiers;
        solvable_tiers = entry;
    }
    // A well-formed tier DAG should have at least one primitive tier.
    assert(solvable_tiers);
}

static Value SolveTierTree(void) {
    TierHashTableEntry *solvable_tail =
        GetTierHashTableEntryListTail(solvable_tiers);
    TierHashTableEntry *tmp;

    while (solvable_tiers != NULL) {
        if (IsCanonicalTier(solvable_tiers->item)) {
            // Only solve canonical tiers.
            TierSolverStatistics stat = TierSolverSolve(solvable_tiers->item);
            if (stat.num_legal_pos > 0) {
                // Solve succeeded.
                UpdateTierHashTable(solvable_tiers->item, &solvable_tail);
                UpdateGlobalStatistics(stat);
                PrintStatistics(solvable_tiers->item, stat);
                ++solved_tiers;
            } else {
                printf("Failed to solve tier %" PRId64 ": not enough memory\n",
                       solvable_tiers->item);
                ++failed_tiers;
            }
        } else {
            ++skipped_tiers;
        }
        tmp = solvable_tiers;
        solvable_tiers = solvable_tiers->next;
        free(tmp);
    }
    PrintSolverResult();

    // TODO: link prober and return value of initial position if possible.
    return kUndecided;
}

static inline int8_t *TierHashTableEntryGetStatus(TierHashTableEntry *entry) {
    return &((TierHashTableEntryExtra *)entry->reserved)->status;
}

static inline int32_t *TierHashTableEntryGetNumUnsolvedChildren(
    TierHashTableEntry *entry) {
    return &((TierHashTableEntryExtra *)entry->reserved)->num_unsolved_children;
}

static TierHashTableEntry *GetTierHashTableEntryListTail(
    TierHashTableEntryList list) {
    if (list == NULL) return NULL;
    while (list->next) {
        list = list->next;
    }
    return list;
}

static void UpdateTierHashTable(Tier solved_tier,
                                TierHashTableEntry **solvable_tail) {
    TierHashTableEntry *tmp;
    TierArray parent_tiers = GetParentTiers(solved_tier);
    TierArray canonical_parents;
    TierArrayInit(&canonical_parents);
    for (int64_t i = 0; i < parent_tiers.size; ++i) {
        // Update canonical parent's number of unsolved children only.
        Tier canonical = GetCanonicalTier(parent_tiers.array[i]);
        if (TierArrayContains(&canonical_parents, canonical)) {
            /* It is possible that a child has two parents that are symmetrical
               to each other. In this case, we should only decrement the child
               counter once. */
            continue;
        }
        TierArrayAppend(&canonical_parents, canonical);
        tmp = TierHashTableFind(&table, canonical);
        if (tmp && --(*TierHashTableEntryGetNumUnsolvedChildren(tmp)) == 0) {
            tmp = TierHashTableRemove(&table, canonical);
            (*solvable_tail)->next = tmp;
            tmp->next = NULL;
            *solvable_tail = tmp;
        }
    }
    TierArrayDestroy(&canonical_parents);
    TierArrayDestroy(&parent_tiers);
}

static void UpdateGlobalStatistics(TierSolverStatistics stat) {
    global_stat.num_win += stat.num_win;
    global_stat.num_lose += stat.num_lose;
    global_stat.num_legal_pos += stat.num_legal_pos;
    if (stat.longest_num_steps_to_p1_win >
        global_stat.longest_num_steps_to_p1_win) {
        global_stat.longest_num_steps_to_p1_win =
            stat.longest_num_steps_to_p1_win;
        global_stat.longest_pos_to_p1_win = stat.longest_pos_to_p1_win;
    }
    if (stat.longest_num_steps_to_p2_win >
        global_stat.longest_num_steps_to_p2_win) {
        global_stat.longest_num_steps_to_p2_win =
            stat.longest_num_steps_to_p2_win;
        global_stat.longest_pos_to_p2_win = stat.longest_pos_to_p2_win;
    }
}

static void PrintStatistics(Tier solved_tier, TierSolverStatistics stat) {
    if (solved_tier > 0) printf("Tier %" PRId64 ":\n", solved_tier);
    printf("total legal positions: %" PRId64 "\n", stat.num_legal_pos);
    printf("number of winning positions: %" PRId64 "\n", stat.num_win);
    printf("number of losing positions: %" PRId64 "\n", stat.num_lose);
    printf("number of drawing positions: %" PRId64 "\n",
           stat.num_legal_pos - stat.num_win - stat.num_lose);
    printf("longest win for player 1 is %" PRId64 " steps at position %" PRId64
           "\n",
           stat.longest_num_steps_to_p1_win, stat.longest_pos_to_p1_win);
    printf("longest win for player 2 is %" PRId64 " steps at position %" PRId64
           "\n\n",
           stat.longest_num_steps_to_p2_win, stat.longest_pos_to_p2_win);
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
    PrintStatistics(-1, global_stat);
    printf("\n");
}

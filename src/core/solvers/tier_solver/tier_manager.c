/**
 * @file tier_manager.c
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Separated functions that
 * handle tier graph traversal and management into its own module, and
 * redesigned retrograde tier analysis process to enable concurrent solving
 * of multiple tiers.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the manager module of the Loopy Tier Solver.
 *
 * @details The tier manager module is responsible for scanning, validating, and
 * creating the tier graph in memory, keeping track of solvable and solved
 * tiers, and dispatching jobs to the tier worker module.
 * @version 1.4.2
 * @date 2024-12-22
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
#include <stdbool.h>   // bool, false
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t
#include <stdio.h>     // printf, fprintf, stderr
#include <stdlib.h>    // malloc, free
#include <time.h>      // time_t, time, difftime

#include "core/analysis/analysis.h"
#include "core/db/db_manager.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/reverse_tier_graph.h"
#include "core/solvers/tier_solver/tier_analyzer.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/solvers/tier_solver/tier_worker.h"
#include "core/types/gamesman_types.h"

#ifdef USE_MPI
#include <mpi.h>

#include "core/solvers/tier_solver/tier_mpi.h"
#endif  // USE_MPI

enum TierManagementType {
    kTierSolving,
    kTierAnalyzing,
};

typedef enum TierGraphNodeStatus {
    kStatusNotVisited,
    kStatusInProgress,
    kStatusClosed,
    kNumStatus
} TierGraphNodeStatus;

enum TierGraphErrorTypes {
    kTierGraphNoError,
    kTierGraphOutOfMemory,
    kTierGraphLoopDetected,
};

// Reference to the API functions from tier_solver.
static const TierSolverApi *api_internal;

// The tier graph that maps each tier to its value. The value of a tier contains
// information about its number of undecided children (or undiscovered parents
// if the graph is reversed) and discovery status. The discovery status is used
// to detect loops in the tier graph during topological sort.
static TierHashMap tier_graph;
static TierQueue pending_tiers;  // Tiers ready to be solved/analyzed.

// Size of the largest tier in number of positions.
static int64_t max_tier_size;

// Largest tier in the tier graph.
static Tier largest_tier;

// Size of the largest "group" of tiers. A group of tiers consists of a parent
// tier and either all of its tier children or its largest child tier, depending
// on the type of the parent tier. This is used to determine the least amount of
// memory needed to solve the game.
static int64_t max_tier_group_size;

// Parent of the largest tier group.
static Tier largest_tier_group_parent;

// Cached reverse tier graph of the game.
static ReverseTierGraph reverse_tier_graph;

static int64_t total_size;
static int64_t total_tiers;
static int64_t total_canonical_tiers;
static int64_t processed_size;
static int64_t processed_tiers;
static int64_t skipped_tiers;
static int64_t failed_tiers;

static Analysis game_analysis;

// Helper functions.

static int InitGlobalVariables(int type);
static TierArray PopParentTiers(Tier child);
static TierArray GetParentTiers(Tier child);
static void DestroyGlobalVariables(void);

static int BuildTierGraph(int type);
static int BuildTierGraphProcessChildren(Tier parent, TierStack *fringe,
                                         int type);
static void BuildTierGraphUpdateAnalysis(Tier parent);
static int EnqueuePrimitiveTiers(void);
static void CreateTierGraphPrintError(int error);

#ifndef USE_MPI
static int SolveTierGraph(bool force, int verbose);
#else   // USE_MPI
static int SolveTierGraphMpi(bool force, int verbose);
static void SolveTierGraphMpiTerminateWorkers(void);
static void SolveTierGraphMpiSolveAll(time_t begin_time, bool force,
                                      int verbose);
static void PrintDispatchMessage(Tier tier, int worker_rank);
#endif  // USE_MPI
static bool SolveUpdateTierGraph(Tier solved_tier);
static void SolveTierGraphPrintTime(Tier tier, double time_elapsed_seconds,
                                    bool solved, bool verbose);
static void PrintSolverResult(double time_elapsed);

static int64_t NumTiersAndStatusToValue(int num_tiers, int status);
static int ValueToStatus(int64_t value);
static int ValueToNumTiers(int64_t value);

static int64_t GetValue(Tier tier);
static int GetStatus(Tier tier);
static int GetNumTiers(Tier tier);

static bool TierGraphSetInitial(Tier tier);
static bool TierGraphSetStatus(Tier tier, int status);
static bool TierGraphSetNumTiers(Tier tier, int num_tiers);
static bool IncrementNumParentTiers(Tier tier);

static bool IsCanonicalTier(Tier tier);

static int DiscoverTierGraph(bool force, int verbose);
static void PrintAnalyzed(Tier tier, const Analysis *analysis, int verbose);
static void AnalyzeUpdateTierGraph(Tier analyzed_tier);
static void PrintAnalyzerResult(void);

static int TestTierGraph(long seed, int64_t test_size);
static void PrintTierGraphAnalysis(void);
static void PrintTestResult(double time_elapsed);

// -----------------------------------------------------------------------------

int TierManagerSolve(const TierSolverApi *api, bool force, int verbose) {
    time_t begin = time(NULL);
    api_internal = api;
    int error = InitGlobalVariables(kTierSolving);
    if (error != 0) {
        fprintf(stderr,
                "TierManagerSolve: initialization failed with code %d.\n",
                error);
        return error;
    }

#ifndef USE_MPI  // If not using MPI
    int ret = SolveTierGraph(force, verbose);
#else   // Using MPI
    int ret = SolveTierGraphMpi(force, verbose);
#endif  // USE_MPI
    DestroyGlobalVariables();

    time_t end = time(NULL);
    if (verbose > 0) {
        printf("Time Elapsed: %d seconds\n", (int)difftime(end, begin));
    }

    return ret;
}

int TierManagerAnalyze(const TierSolverApi *api, bool force, int verbose) {
    api_internal = api;
    int error = InitGlobalVariables(kTierAnalyzing);
    if (error != 0) {
        fprintf(stderr,
                "TierManagerAnalyze: initialization failed with code %d.\n",
                error);
        return error;
    }

    int ret = DiscoverTierGraph(force, verbose);
    DestroyGlobalVariables();

    return ret;
}

int TierManagerTest(const TierSolverApi *api, long seed, int64_t test_size) {
    api_internal = api;
    int error = InitGlobalVariables(kTierSolving);
    if (error != 0) {
        fprintf(stderr,
                "TierManagerTest: initialization failed with code %d.\n",
                error);
        return kTierSolverTestDependencyError;
    }
    PrintTierGraphAnalysis();

    int ret = TestTierGraph(seed, test_size);
    DestroyGlobalVariables();

    return ret;
}

// -----------------------------------------------------------------------------

static int InitGlobalVariables(int type) {
    max_tier_size = -1;
    largest_tier = kIllegalTier;
    max_tier_group_size = -1;
    largest_tier_group_parent = kIllegalTier;

    total_size = 0;
    total_tiers = 0;
    total_canonical_tiers = 0;
    processed_size = 0;
    processed_tiers = 0;
    skipped_tiers = 0;
    failed_tiers = 0;
    TierQueueInit(&pending_tiers);
    TierHashMapInit(&tier_graph, 0.5);
    ReverseTierGraphInit(&reverse_tier_graph);
    if (type == kTierAnalyzing) {
        AnalysisInit(&game_analysis);
        AnalysisSetHashSize(&game_analysis, 0);
    }

    return BuildTierGraph(type);
}

static TierArray PopParentTiers(Tier child) {
    return ReverseTierGraphPopParentsOf(&reverse_tier_graph, child);
}

static TierArray GetParentTiers(Tier child) {
    return ReverseTierGraphGetParentsOf(&reverse_tier_graph, child);
}

static void DestroyGlobalVariables(void) {
    TierHashMapDestroy(&tier_graph);
    ReverseTierGraphDestroy(&reverse_tier_graph);
    TierQueueDestroy(&pending_tiers);
}

/**
 * @brief DFS from initial tier with loop detection.
 *
 * @details Iterative topological sort using DFS and node coloring (status
 * marking). Algorithm by Ctrl, stackoverflow.com.
 * @link https://stackoverflow.com/a/73210346
 */
static int BuildTierGraph(int type) {
    int ret = 1;
    TierStack fringe;
    TierStackInit(&fringe);
    Tier initial_tier = api_internal->GetInitialTier();
    if (!TierStackPush(&fringe, initial_tier)) goto _bailout;
    if (!TierGraphSetInitial(initial_tier)) goto _bailout;

    while (!TierStackEmpty(&fringe)) {
        Tier parent = TierStackTop(&fringe);
        int status = GetStatus(parent);
        if (status == kStatusInProgress) {
            if (!TierGraphSetStatus(parent, kStatusClosed)) goto _bailout;
            TierStackPop(&fringe);
            continue;
        } else if (status == kStatusClosed) {
            TierStackPop(&fringe);
            continue;
        }
        if (!TierGraphSetStatus(parent, kStatusInProgress)) goto _bailout;
        int error = BuildTierGraphProcessChildren(parent, &fringe, type);
        if (error != kTierGraphNoError) {
            ret = error;
            goto _bailout;
        }
    }
    ret = 0;

_bailout:
    TierStackDestroy(&fringe);
    if (ret != 0) {
        TierHashMapDestroy(&tier_graph);
        ReverseTierGraphDestroy(&reverse_tier_graph);
        CreateTierGraphPrintError(ret);
    } else if (type == kTierSolving) {
        EnqueuePrimitiveTiers();
    } else {  // type == kTierAnalyzing
        TierQueuePush(&pending_tiers, initial_tier);
    }

    return ret;
}

/**
 * @brief Returns an array of unique canonical child tiers of tier \p parent.
 * Uniqueness enforced through deduplication of child tiers that are symmetric
 * to each other.
 */
static TierArray GetCanonicalChildTiers(Tier parent) {
    TierArray ret;
    TierArrayInit(&ret);
    TierHashSet dedup;
    TierHashSetInit(&dedup, 0.5);
    TierArray children = api_internal->GetChildTiers(parent);
    for (int64_t i = 0; i < children.size; ++i) {
        Tier canonical = api_internal->GetCanonicalTier(children.array[i]);
        if (TierHashSetContains(&dedup, canonical)) continue;

        TierHashSetAdd(&dedup, canonical);
        TierArrayAppend(&ret, canonical);
    }
    TierArrayDestroy(&children);
    TierHashSetDestroy(&dedup);

    return ret;
}

/**
 * @brief Returns the number of unique canonical child tiers of tier \p parent
 * in the array of tier \p children, or -1 if there is a duplicate in
 * \p children.
 */
static int GetNumCanonicalChildTiers(Tier parent, const TierArray *children) {
    int ret = 0;
    TierHashSet dedup, canonical_dedup;
    TierHashSetInit(&dedup, 0.5);
    TierHashSetInit(&canonical_dedup, 0.5);
    int64_t i;
    for (i = 0; i < children->size; ++i) {  // For each child
        if (TierHashSetContains(&dedup, children->array[i])) {
            ret = -1;
            break;
        }
        TierHashSetAdd(&dedup, children->array[i]);

        Tier canonical = api_internal->GetCanonicalTier(children->array[i]);
        if (!TierHashSetContains(&canonical_dedup, canonical)) {
            TierHashSetAdd(&canonical_dedup, canonical);
            ++ret;
        }
    }
    TierHashSetDestroy(&dedup);
    TierHashSetDestroy(&canonical_dedup);

    if (ret < 0) {
        char name[kDbFileNameLengthMax + 1];
        api_internal->GetTierName(parent, name);
        printf("ERROR: tier [%s] (#%" PRITier
               ") contains duplicate tier children\n",
               name, parent);
        api_internal->GetTierName(children->array[i], name);
        printf("The duplicated child tier is [%s] (#%" PRITier ")\n", name,
               children->array[i]);
        printf("List of all child tiers:\n");
        for (int64_t j = 0; j < children->size; ++j) {
            api_internal->GetTierName(children->array[j], name);
            printf("[%s] (#%" PRITier ")\n", name, children->array[j]);
        }
        printf("\n");
    }

    return ret;
}

static int BuildTierGraphProcessChildren(Tier parent, TierStack *fringe,
                                         int type) {
    // Add tier size to total if it is canonical.
    ++total_tiers;
    if (IsCanonicalTier(parent)) {
        ++total_canonical_tiers;
        total_size += api_internal->GetTierSize(parent);
    }

    TierArray children = api_internal->GetChildTiers(parent);
    BuildTierGraphUpdateAnalysis(parent);
    int num_canonical_tier_children =
        GetNumCanonicalChildTiers(parent, &children);
    if (num_canonical_tier_children < 0) return kIllegalGameTierGraphError;

    if (type == kTierSolving) {
        if (!TierGraphSetNumTiers(parent, num_canonical_tier_children)) {
            TierArrayDestroy(&children);
            return (int)kTierGraphOutOfMemory;
        }
    } else {  // type == kTierAnalyzing
        for (int64_t i = 0; i < children.size; ++i) {
            Tier child = children.array[i];
            if (!IncrementNumParentTiers(child)) {
                TierArrayDestroy(&children);
                return (int)kTierGraphOutOfMemory;
            }
        }
    }

    for (int64_t i = 0; i < children.size; ++i) {
        Tier child = children.array[i];
        if (ReverseTierGraphAdd(&reverse_tier_graph, child, parent) != 0) {
            TierArrayDestroy(&children);
            return (int)kTierGraphOutOfMemory;
        }
        if (!TierHashMapContains(&tier_graph, child)) {
            if (!TierGraphSetInitial(child)) {
                TierArrayDestroy(&children);
                return (int)kTierGraphOutOfMemory;
            }
        }
        int status = GetStatus(child);
        if (status == kStatusNotVisited) {
            TierStackPush(fringe, child);
        } else if (status == kStatusInProgress) {
            TierArrayDestroy(&children);
            return (int)kTierGraphLoopDetected;
        }  // else, child tier is already closed and we take no action.
    }

    TierArrayDestroy(&children);
    return (int)kTierGraphNoError;
}

static void BuildTierGraphUpdateAnalysis(Tier parent) {
    // If the parent tier is not canonical, then it is never solved.
    // So, there is no need to consider it in the analysis.
    if (!IsCanonicalTier(parent)) return;

    // Check if this is the largest tier.
    int64_t total_size = api_internal->GetTierSize(parent);
    if (total_size > max_tier_size) {
        max_tier_size = total_size;
        largest_tier = parent;
    }

    // Check if this is the largest group of tiers.
    TierArray canonical_children = GetCanonicalChildTiers(parent);
    if (api_internal->GetTierType(parent) == kTierTypeImmediateTransition) {
        int64_t largest_child_size = 0;
        for (int64_t i = 0; i < canonical_children.size; ++i) {
            int64_t this_child_size =
                api_internal->GetTierSize(canonical_children.array[i]);
            if (this_child_size > largest_child_size) {
                largest_child_size = this_child_size;
            }
        }
        total_size += largest_child_size;
    } else {
        for (int64_t i = 0; i < canonical_children.size; ++i) {
            total_size +=
                api_internal->GetTierSize(canonical_children.array[i]);
        }
    }
    TierArrayDestroy(&canonical_children);

    if (total_size > max_tier_group_size) {
        max_tier_group_size = total_size;
        largest_tier_group_parent = parent;
    }
}

static int EnqueuePrimitiveTiers(void) {
    TierHashMapIterator it = TierHashMapBegin(&tier_graph);
    Tier tier;
    int64_t value;
    while (TierHashMapIteratorNext(&it, &tier, &value)) {
        if (ValueToNumTiers(value) == 0) {
            if (!TierQueuePush(&pending_tiers, tier)) {
                return kMallocFailureError;
            }
        }
    }

    if (TierQueueEmpty(&pending_tiers)) {
        fprintf(stderr,
                "EnqueuePrimitiveTiers: (BUG) The tier graph contains no "
                "primitive tiers.\n");
        return kIllegalGameTierGraphError;
    }
    return kNoError;
}

static void CreateTierGraphPrintError(int error) {
    switch (error) {
        case kNoError:
            break;

        case kTierGraphOutOfMemory:
            fprintf(stderr, "BuildTierGraph: out of memory.\n");
            break;

        case kTierGraphLoopDetected:
            fprintf(stderr,
                    "BuildTierGraph: a loop is detected in the tier graph.\n");
            break;
    }
}

#ifndef USE_MPI

static int SolveTierGraph(bool force, int verbose) {
    TierWorkerSolveOptions options = {
        .compare = false,
        .force = force,
        .verbose = verbose,
    };
    double time_elapsed = 0.0;
    if (verbose > 0) {
        printf("Begin solving all %" PRId64 " tiers (%" PRId64
               " canonical) of total size %" PRId64 " (positions)\n",
               total_tiers, total_canonical_tiers, total_size);
    }

    while (!TierQueueEmpty(&pending_tiers)) {
        Tier tier = TierQueuePop(&pending_tiers);
        if (IsCanonicalTier(tier)) {  // Only solve canonical tiers.
            time_t begin = time(NULL);
            bool solved;
            TierType type = api_internal->GetTierType(tier);
            int error = TierWorkerSolve(GetMethodForTierType(type), tier,
                                        &options, &solved);
            if (error == 0) {
                // Solve succeeded.
                SolveUpdateTierGraph(tier);
                ++processed_tiers;
            } else {
                printf("Failed to solve tier %" PRITier ", code %d\n", tier,
                       error);
                ++failed_tiers;
            }
            time_t end = time(NULL);
            time_elapsed += difftime(end, begin);
            SolveTierGraphPrintTime(tier, time_elapsed, solved, verbose);
        } else {
            ++skipped_tiers;
        }
    }
    if (verbose > 0) PrintSolverResult(time_elapsed);
    if (failed_tiers == 0) {
        int error = DbManagerSetGameSolved();
        if (error != kNoError) {
            fprintf(stderr,
                    "SolveTierGraph: DB manager failed to set current game as "
                    "solved (code %d)\n",
                    error);
            return error;
        }
    }

    return kNoError;
}

#else  // USE_MPI

static int SolveTierGraphMpi(bool force, int verbose) {
    if (verbose > 0) {
        printf("Begin solving all %" PRId64 " tiers (%" PRId64
               " canonical) of total size %" PRId64 " (positions)\n",
               total_tiers, total_canonical_tiers, total_size);
    }

    time_t begin_time = time(NULL);
    SolveTierGraphMpiSolveAll(begin_time, force, verbose);
    SolveTierGraphMpiTerminateWorkers();
    double time_elapsed = difftime(time(NULL), begin_time);
    if (verbose > 0) PrintSolverResult(time_elapsed);
    if (failed_tiers == 0) {
        int error = DbManagerSetGameSolved();
        if (error != kNoError) {
            fprintf(
                stderr,
                "SolveTierGraphMpi: DB manager failed to set current game as "
                "solved (code %d)\n",
                error);
            return error;
        }
    }

    return kNoError;
}

static void SolveTierGraphMpiTerminateWorkers(void) {
    int num_workers = SafeMpiCommSize(MPI_COMM_WORLD) - 1, num_terminated = 0;

    while (num_terminated < num_workers) {
        TierMpiWorkerMessage worker_msg;
        int worker_rank;
        TierMpiManagerRecvAnySource(&worker_msg, &worker_rank);
        TierMpiManagerSendTerminate(worker_rank);
        ++num_terminated;
    }
}

static void SolveTierGraphMpiSolveAll(time_t begin_time, bool force,
                                      int verbose) {
    static Tier job_list[kMpiNumNodesMax];
    static TierArray solving_tiers;
    TierArrayInit(&solving_tiers);

    while (!TierQueueEmpty(&pending_tiers) || !TierArrayEmpty(&solving_tiers)) {
        TierMpiWorkerMessage worker_msg;
        int worker_rank;
        TierMpiManagerRecvAnySource(&worker_msg, &worker_rank);
        if (worker_msg.request != kTierMpiRequestCheck) {  // Not just checking.
            bool solved = (worker_msg.request == kTierMpiRequestReportSolved);
            Tier tier = job_list[worker_rank];
            if (worker_msg.request == kTierMpiRequestReportError) {  // Failed.
                printf("Failed to solve tier %" PRITier ", code %d\n", tier,
                       worker_msg.error);
                ++failed_tiers;
            } else {  // Successfully solved or loaded.
                SolveUpdateTierGraph(tier);
                ++processed_tiers;
            }
            TierArrayRemove(&solving_tiers, tier);

            double time_elapsed = difftime(time(NULL), begin_time);
            SolveTierGraphPrintTime(tier, time_elapsed, solved, verbose);
        }
        // The worker node that we received a message from is now idle.

        // Keep popping off non-nanonical tiers from the front of the pending
        // tier queue until we see the first canonical one or the list becomes
        // empty.
        while (!TierQueueEmpty(&pending_tiers) &&
               !IsCanonicalTier(TierQueueFront(&pending_tiers))) {
            ++skipped_tiers;
            TierQueuePop(&pending_tiers);
        }
        if (!TierQueueEmpty(&pending_tiers)) {
            // A solvable tier is available, dispatch it to the worker node.
            Tier tier = TierQueuePop(&pending_tiers);
            PrintDispatchMessage(tier, worker_rank);
            job_list[worker_rank] = tier;
            TierMpiManagerSendSolve(worker_rank, tier, force);
            TierArrayAppend(&solving_tiers, tier);
        } else {
            // No solvable tiers available, let the worker node go to sleep.
            TierMpiManagerSendSleep(worker_rank);
        }
    }

    TierArrayDestroy(&solving_tiers);
}

static void PrintDispatchMessage(Tier tier, int worker_rank) {
    char tier_name[kDbFileNameLengthMax + 1];
    api_internal->GetTierName(tier, tier_name);
    printf("Dispatching tier [%s] (#%" PRITier ") to worker %d.\n", tier_name,
           tier, worker_rank);
    fflush(stdout);
}

#endif  // USE_MPI

static bool SolveUpdateTierGraph(Tier solved_tier) {
    TierArray parent_tiers = PopParentTiers(solved_tier);
    TierHashSet canonical_parents;
    TierHashSetInit(&canonical_parents, 0.5);
    for (int64_t i = 0; i < parent_tiers.size; ++i) {
        // Update canonical parent's number of unsolved children only.
        Tier canonical = api_internal->GetCanonicalTier(parent_tiers.array[i]);
        if (TierHashSetContains(&canonical_parents, canonical)) {
            // It is possible that a child has two parents that are symmetrical
            // to each other. In this case, we should only decrement the child
            // counter once.
            continue;
        }
        TierHashSetAdd(&canonical_parents, canonical);
        int num_unsolved_child_tiers = GetNumTiers(canonical);
        if (num_unsolved_child_tiers <= 0) {
            char name[kDbFileNameLengthMax + 1];
            api_internal->GetTierName(canonical, name);
            printf(
                "SolveUpdateTierGraph: ERROR - attempting to reduce the number "
                "of unsolved children of tier [%s] (#%" PRITier
                ") below zero. This typically indicates a bug in the game's "
                "tier symmetry removal code.\n",
                name, canonical);
            return false;
        }

        bool success =
            TierGraphSetNumTiers(canonical, num_unsolved_child_tiers - 1);
        assert(success);
        (void)success;
        if (num_unsolved_child_tiers == 1) {
            TierQueuePush(&pending_tiers, canonical);
        }
    }
    TierHashSetDestroy(&canonical_parents);
    TierArrayDestroy(&parent_tiers);

    return true;
}

static void SolveTierGraphPrintTime(Tier tier, double time_elapsed_seconds,
                                    bool solved, bool verbose) {
    int64_t tier_size = api_internal->GetTierSize(tier);
    if (verbose > 0) printf("%s: ", GetTimeStampString());
    ReadOnlyString operation;
    if (solved) {
        processed_size += tier_size;
        operation = "solving";
    } else {
        total_size -= tier_size;
        operation = "checking";
    }
    if (verbose > 0) {
        char tier_name[kDbFileNameLengthMax + 1];
        api_internal->GetTierName(tier, tier_name);
        printf("Finished %s tier [%s] ", operation, tier_name);
        printf("(#%" PRITier ") of size %" PRId64 ", ", tier, tier_size);

        int64_t remaining_size = total_size - processed_size;
        printf("remaining size %" PRId64 ". ", remaining_size);

        printf("Current speed: ");
        if (time_elapsed_seconds > 0.0) {
            double speed = (double)processed_size / time_elapsed_seconds;
            printf("%f positions/sec. ", speed);
        } else {
            printf("N/A. ");
        }

        ReadOnlyString time_string;
        if (processed_size > 0) {
            double time_remaining = time_elapsed_seconds /
                                    (double)processed_size *
                                    (double)remaining_size;
            time_string = SecondsToFormattedTimeString(time_remaining);
        } else {
            time_string = "unknown";
        }
        printf("Estimated time remaining: %s.\n", time_string);
    }
    fflush(stdout);
}

static void PrintSolverResult(double time_elapsed) {
    printf(
        "Finished solving all tiers in %d second(s).\n"
        "Number of canonical tiers solved: %" PRId64
        "\nNumber of non-canonical tiers skipped: %" PRId64
        "\nNumber of tiers failed due to OOM: %" PRId64
        "\nTotal tiers scanned: %" PRId64 "\n",
        (int)time_elapsed, processed_tiers, skipped_tiers, failed_tiers,
        processed_tiers + skipped_tiers + failed_tiers);
    printf("\n");
}

static int64_t NumTiersAndStatusToValue(int num_tiers, int status) {
    return num_tiers * kNumStatus + status;
}

static int ValueToStatus(int64_t value) { return (int)(value % kNumStatus); }

static int ValueToNumTiers(int64_t value) { return (int)(value / kNumStatus); }

static int64_t GetValue(Tier tier) {
    TierHashMapIterator it = TierHashMapGet(&tier_graph, tier);
    if (!TierHashMapIteratorIsValid(&it)) return -1;
    return TierHashMapIteratorValue(&it);
}

static int GetStatus(Tier tier) { return ValueToStatus(GetValue(tier)); }

static int GetNumTiers(Tier tier) { return ValueToNumTiers(GetValue(tier)); }

static bool TierGraphSetInitial(Tier tier) {
    assert(!TierHashMapContains(&tier_graph, tier));
    int64_t value = NumTiersAndStatusToValue(0, kStatusNotVisited);
    return TierHashMapSet(&tier_graph, tier, value);
}

static bool TierGraphSetStatus(Tier tier, int status) {
    int64_t value = GetValue(tier);
    int num_tiers = ValueToNumTiers(value);
    value = NumTiersAndStatusToValue(num_tiers, status);
    return TierHashMapSet(&tier_graph, tier, value);
}

static bool TierGraphSetNumTiers(Tier tier, int num_tiers) {
    int64_t value = GetValue(tier);
    int status = ValueToStatus(value);
    value = NumTiersAndStatusToValue(num_tiers, status);
    return TierHashMapSet(&tier_graph, tier, value);
}

static bool IncrementNumParentTiers(Tier tier) {
    int64_t value = GetValue(tier);
    if (value < 0) value = NumTiersAndStatusToValue(0, kStatusNotVisited);

    int status = ValueToStatus(value);
    int num_tiers = ValueToNumTiers(value) + 1;
    value = NumTiersAndStatusToValue(num_tiers, status);
    return TierHashMapSet(&tier_graph, tier, value);
}

static bool IsCanonicalTier(Tier tier) {
    return api_internal->GetCanonicalTier(tier) == tier;
}

static int DiscoverTierGraph(bool force, int verbose) {
    TierAnalyzerInit(api_internal);
    while (!TierQueueEmpty(&pending_tiers)) {
        Tier tier = TierQueuePop(&pending_tiers);
        Tier canonical = api_internal->GetCanonicalTier(tier);

        // Analyze the canonical tier instead.
        Analysis *tier_analysis = (Analysis *)malloc(sizeof(Analysis));
        if (tier_analysis == NULL) {
            TierAnalyzerFinalize();
            return kMallocFailureError;
        }
        AnalysisInit(tier_analysis);

        int error = TierAnalyzerAnalyze(tier_analysis, canonical, force);
        if (error == 0) {
            // Analyzer succeeded.
            PrintAnalyzed(tier, tier_analysis, verbose);
            AnalyzeUpdateTierGraph(tier);
            ++processed_tiers;

            // If tier is non-canonical, we must convert the analysis to non-
            // cannonical. Note that the analysis here is the analysis of the
            // canonical tier on disk.
            if (tier != canonical) {
                AnalysisConvertToNoncanonical(
                    tier_analysis, tier,
                    api_internal->GetPositionInSymmetricTier);
            }
            AnalysisAggregate(&game_analysis, tier_analysis);
        } else {
            printf("Failed to analyze tier %" PRITier ", code %d\n", tier,
                   error);
            ++failed_tiers;
        }
        free(tier_analysis);
    }

    if (verbose > 0) PrintAnalyzerResult();
    TierAnalyzerFinalize();
    return kNoError;
}

static void PrintAnalyzed(Tier tier, const Analysis *analysis, int verbose) {
    char tier_name[kDbFileNameLengthMax + 1];
    api_internal->GetTierName(tier, tier_name);
    if (verbose > 0) {
        printf("\n--- Tier [%s] (#%" PRITier ") analyzed ---\n", tier_name,
               tier);
    }
    if (verbose > 1) {
        AnalysisPrintEverything(stdout, analysis);
    } else if (verbose > 0) {
        AnalysisPrintStatistics(stdout, analysis);
    }
}

static void AnalyzeUpdateTierGraph(Tier analyzed_tier) {
    TierArray child_tiers = api_internal->GetChildTiers(analyzed_tier);
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        Tier child = child_tiers.array[i];
        int num_undiscovered_parent_tiers = GetNumTiers(child);
        assert(num_undiscovered_parent_tiers > 0);
        if (!TierGraphSetNumTiers(child, num_undiscovered_parent_tiers - 1)) {
            NotReached(
                "AnalyzeUpdateTierGraph: unexpected error while resetting an "
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
        "\n--- Finished analyzing all tiers. ---\n"
        "Number of canonical tiers analyzed: %" PRId64
        "\nNumber of tiers failed due to OOM: %" PRId64
        "\nTotal tiers scanned: %" PRId64 "\n\n",
        processed_tiers, failed_tiers, processed_tiers + failed_tiers);

    if (failed_tiers > 0) {
        printf(
            "DiscoverTierGraph: (WARNING) At least one tier failed to be "
            "analyzed and the analysis of the game may be inaccurate.\n");
    }

    AnalysisPrintEverything(stdout, &game_analysis);
}

static int TestTierGraph(long seed, int64_t test_size) {
    double time_elapsed = 0.0;
    printf("Begin random sanity testing of all %" PRId64 " tiers (%" PRId64
           " canonical) of total size %" PRId64 " (positions). %" PRId64
           " tiers are primitive.\n",
           total_tiers, total_canonical_tiers, total_size,
           TierQueueSize(&pending_tiers));

    char tier_name[kDbFileNameLengthMax + 1];
    while (!TierQueueEmpty(&pending_tiers)) {
        Tier tier = TierQueuePop(&pending_tiers);
        if (!IsCanonicalTier(tier)) {  // Only test canonical tiers.
            ++skipped_tiers;
            continue;
        }

        time_t begin = time(NULL);
        int error = api_internal->GetTierName(tier, tier_name);
        if (error != kNoError) {
            printf("Failed to get name of tier %" PRITier "\n", tier);
            return kTierSolverTestGetTierNameError;
        }

        printf("Testing tier [%s] (#%" PRITier ") of size %" PRId64 "... ",
               tier_name, tier, api_internal->GetTierSize(tier));
        TierArray parent_tiers = GetParentTiers(tier);
        TierArrayAppend(&parent_tiers, tier);
        error = TierWorkerTest(tier, &parent_tiers, seed, test_size);
        TierArrayDestroy(&parent_tiers);
        if (error == kTierSolverTestNoError) {
            // Test passed.
            if (!SolveUpdateTierGraph(tier)) return kIllegalGameTierGraphError;
            ++processed_tiers;
        } else {
            printf("FAILED\n");
            return error;
        }
        time_t end = time(NULL);
        time_elapsed += difftime(end, begin);
        printf("PASSED. %" PRId64 " tiers ready in test queue\n",
               TierQueueSize(&pending_tiers));
    }
    PrintTestResult(time_elapsed);

    return kTierSolverTestNoError;
}

static void PrintTierGraphAnalysis(void) {
    char name[kDbFileNameLengthMax + 1];

    // Report on the largest canonical tier.
    printf("Finished building the tier graph.\n");
    api_internal->GetTierName(largest_tier, name);
    printf("The largest canonical tier is [%s] (#%" PRITier
           "), which contains %" PRId64 " positions.\n",
           name, largest_tier, max_tier_size);

    // Report on the largest canonical tier group.
    api_internal->GetTierName(largest_tier_group_parent, name);
    printf(
        "The largest canonical tier group, whose parent tier is [%s] "
        "(#%" PRITier "), contains %" PRId64 " positions.\n",
        name, largest_tier_group_parent, max_tier_group_size);
    printf(
        "The largest canonical tier group contains the following child "
        "tiers:\n");
    TierArray children = api_internal->GetChildTiers(largest_tier_group_parent);
    TierHashSet dedup;
    TierHashSetInit(&dedup, 0.5);
    for (int64_t i = 0; i < children.size; ++i) {
        api_internal->GetTierName(children.array[i], name);
        printf("[%s] (#%" PRITier "), ", name, children.array[i]);
        const Tier canonical =
            api_internal->GetCanonicalTier(children.array[i]);
        api_internal->GetTierName(canonical, name);
        if (TierHashSetContains(&dedup, canonical)) {
            printf("which is already loaded as [%s] (#%" PRITier ")\n", name,
                   canonical);
        } else {
            TierHashSetAdd(&dedup, canonical);
            int64_t size = api_internal->GetTierSize(canonical);
            if (canonical == children.array[i]) {
                printf("which is canonical and contains %" PRId64
                       " positions\n",
                       size);
            } else {
                printf("which will be loaded as [%s] (#%" PRITier
                       ") of size %" PRId64 " positions\n",
                       name, canonical, size);
            }
        }
    }
    TierHashSetDestroy(&dedup);
    TierArrayDestroy(&children);
}

static void PrintTestResult(double time_elapsed) {
    assert(failed_tiers == 0);
    printf(
        "Finished testing all tiers in %d second(s).\n"
        "Number of canonical tiers tested: %" PRId64
        "\nNumber of non-canonical tiers skipped: %" PRId64
        "\nTotal tiers tested: %" PRId64 "\n",
        (int)time_elapsed, processed_tiers, skipped_tiers,
        processed_tiers + skipped_tiers);
    printf("\n");
}

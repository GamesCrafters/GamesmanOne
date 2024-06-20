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
 * @version 1.2.0
 * @date 2024-03-18
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
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t
#include <stdio.h>     // fprintf, stderr
#include <stdlib.h>    // malloc, free
#include <string.h>    // memcpy
#include <time.h>      // time_t, time, difftime

#include "core/analysis/analysis.h"
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

enum TierTreeErrorTypes {
    kTierTreeNoError,
    kTierTreeOutOfMemory,
    kTierTreeLoopDetected,
};

// Copy of the API functions from tier_solver. Cannot use a reference here
// because we need to create/modify some of the functions.
static TierSolverApi current_api;

// The tier tree that maps each tier to its value. The value of a tier contains
// information about its number of undecided children (or undiscovered parents
// if the tree is reversed) and discovery status. The discovery status is used
// to detect loops in the tier graph during topological sort.
static TierHashMap tier_tree;
static TierQueue pending_tiers;  // Tiers ready to be solved/analyzed.

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

static int BuildTierTree(int type);
static int BuildTierTreeProcessChildren(Tier parent, TierStack *fringe,
                                        int type);
static int EnqueuePrimitiveTiers(void);
static void CreateTierTreePrintError(int error);

#ifndef USE_MPI
static int SolveTierTree(bool force, int verbose);
#else   // USE_MPI
static int SolveTierTreeMpi(bool force, int verbose);
static void SolveTierTreeMpiTerminateWorkers(void);
static void SolveTierTreeMpiSolveAll(time_t begin_time, bool force,
                                     int verbose);
static void PrintDispatchMessage(Tier tier, int worker_rank);
#endif  // USE_MPI
static void SolveUpdateTierTree(Tier solved_tier);
static void SolveTierTreePrintTime(Tier tier, double time_elapsed_seconds,
                                   bool solved, bool verbose);
static void PrintSolverResult(double time_elapsed);

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

static int DiscoverTierTree(bool force, int verbose);
static void PrintAnalyzed(Tier tier, const Analysis *analysis, int verbose);
static void AnalyzeUpdateTierTree(Tier analyzed_tier);
static void PrintAnalyzerResult(void);

static int TestTierTree(long seed);
static void PrintTestResult(double time_elapsed);

// -----------------------------------------------------------------------------

int TierManagerSolve(const TierSolverApi *api, bool force, int verbose) {
    time_t begin = time(NULL);
    memcpy(&current_api, api, sizeof(current_api));
    int error = InitGlobalVariables(kTierSolving);
    if (error != 0) {
        fprintf(stderr,
                "TierManagerSolve: initialization failed with code %d.\n",
                error);
        return error;
    }

#ifndef USE_MPI
    int ret = SolveTierTree(force, verbose);
#else   // USE_MPI
    int ret = SolveTierTreeMpi(force, verbose);
#endif  // USE_MPI
    DestroyGlobalVariables();

    time_t end = time(NULL);
    if (verbose > 0) {
        printf("Time Elapsed: %f seconds\n", difftime(end, begin));
    }

    return ret;
}

int TierManagerAnalyze(const TierSolverApi *api, bool force, int verbose) {
    memcpy(&current_api, api, sizeof(current_api));
    int error = InitGlobalVariables(kTierAnalyzing);
    if (error != 0) {
        fprintf(stderr,
                "TierManagerAnalyze: initialization failed with code %d.\n",
                error);
        return error;
    }

    int ret = DiscoverTierTree(force, verbose);
    DestroyGlobalVariables();

    return ret;
}

int TierManagerTest(const TierSolverApi *api, long seed) {
    memcpy(&current_api, api, sizeof(current_api));
    int error = InitGlobalVariables(kTierSolving);
    if (error != 0) {
        fprintf(stderr,
                "TierManagerTest: initialization failed with code %d.\n",
                error);
        return kTierSolverTestDependencyError;
    }

    int ret = TestTierTree(seed);
    DestroyGlobalVariables();

    return ret;
}

// -----------------------------------------------------------------------------

static int InitGlobalVariables(int type) {
    total_size = 0;
    total_tiers = 0;
    total_canonical_tiers = 0;
    processed_size = 0;
    processed_tiers = 0;
    skipped_tiers = 0;
    failed_tiers = 0;
    TierQueueInit(&pending_tiers);
    TierHashMapInit(&tier_tree, 0.5);
    ReverseTierGraphInit(&reverse_tier_graph);
    if (type == kTierAnalyzing) {
        AnalysisInit(&game_analysis);
        AnalysisSetHashSize(&game_analysis, 0);
    }

    return BuildTierTree(type);
}

static TierArray PopParentTiers(Tier child) {
    return ReverseTierGraphPopParentsOf(&reverse_tier_graph, child);
}

static TierArray GetParentTiers(Tier child) {
    return ReverseTierGraphGetParentsOf(&reverse_tier_graph, child);
}

static void DestroyGlobalVariables(void) {
    TierHashMapDestroy(&tier_tree);
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
static int BuildTierTree(int type) {
    int ret = 1;
    TierStack fringe;
    TierStackInit(&fringe);
    Tier initial_tier = current_api.GetInitialTier();
    if (!TierStackPush(&fringe, initial_tier)) goto _bailout;
    if (!TierTreeSetInitial(initial_tier)) goto _bailout;

    while (!TierStackEmpty(&fringe)) {
        Tier parent = TierStackTop(&fringe);
        int status = GetStatus(parent);
        if (status == kStatusInProgress) {
            if (!TierTreeSetStatus(parent, kStatusClosed)) goto _bailout;
            TierStackPop(&fringe);
            continue;
        } else if (status == kStatusClosed) {
            TierStackPop(&fringe);
            continue;
        }
        if (!TierTreeSetStatus(parent, kStatusInProgress)) goto _bailout;
        int error = BuildTierTreeProcessChildren(parent, &fringe, type);
        if (error != kTierTreeNoError) {
            ret = error;
            goto _bailout;
        }
    }
    ret = 0;

_bailout:
    TierStackDestroy(&fringe);
    if (ret != 0) {
        TierHashMapDestroy(&tier_tree);
        ReverseTierGraphDestroy(&reverse_tier_graph);
        CreateTierTreePrintError(ret);
    } else if (type == kTierSolving) {
        EnqueuePrimitiveTiers();
    } else {  // type == kTierAnalyzing
        TierQueuePush(&pending_tiers, initial_tier);
    }
    return ret;
}

static int GetNumCanonicalTiers(const TierArray *array) {
    int ret = 0;
    for (int64_t i = 0; i < array->size; ++i) {
        ret += IsCanonicalTier(array->array[i]);
    }

    return ret;
}

static int BuildTierTreeProcessChildren(Tier parent, TierStack *fringe,
                                        int type) {
    // Add tier size to total if it is canonical.
    ++total_tiers;
    if (IsCanonicalTier(parent)) {
        ++total_canonical_tiers;
        total_size += current_api.GetTierSize(parent);
    }

    TierArray tier_children = current_api.GetChildTiers(parent);
    int num_canonical_tier_children = GetNumCanonicalTiers(&tier_children);
    if (type == kTierSolving) {
        if (!TierTreeSetNumTiers(parent, num_canonical_tier_children)) {
            TierArrayDestroy(&tier_children);
            return (int)kTierTreeOutOfMemory;
        }
    } else {  // type == kTierAnalyzing
        for (int64_t i = 0; i < tier_children.size; ++i) {
            Tier child = tier_children.array[i];
            if (!IncrementNumParentTiers(child)) {
                TierArrayDestroy(&tier_children);
                return (int)kTierTreeOutOfMemory;
            }
        }
    }

    for (int64_t i = 0; i < tier_children.size; ++i) {
        Tier child = tier_children.array[i];
        if (ReverseTierGraphAdd(&reverse_tier_graph, child, parent) != 0) {
            TierArrayDestroy(&tier_children);
            return (int)kTierTreeOutOfMemory;
        }
        if (!TierHashMapContains(&tier_tree, child)) {
            if (!TierTreeSetInitial(child)) {
                TierArrayDestroy(&tier_children);
                return (int)kTierTreeOutOfMemory;
            }
        }
        int status = GetStatus(child);
        if (status == kStatusNotVisited) {
            TierStackPush(fringe, child);
        } else if (status == kStatusInProgress) {
            TierArrayDestroy(&tier_children);
            return (int)kTierTreeLoopDetected;
        }  // else, child tier is already closed and we take no action.
    }

    TierArrayDestroy(&tier_children);
    return (int)kTierTreeNoError;
}

static int EnqueuePrimitiveTiers(void) {
    TierHashMapIterator it = TierHashMapBegin(&tier_tree);
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

static void CreateTierTreePrintError(int error) {
    switch (error) {
        case kNoError:
            break;

        case kTierTreeOutOfMemory:
            fprintf(stderr, "BuildTierTree: out of memory.\n");
            break;

        case kTierTreeLoopDetected:
            fprintf(stderr,
                    "BuildTierTree: a loop is detected in the tier graph.\n");
            break;
    }
}

#ifndef USE_MPI
static int SolveTierTree(bool force, int verbose) {
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
            int error = TierWorkerSolveValueIteration(tier, force, false, &solved);
            if (error == 0) {
                // Solve succeeded.
                SolveUpdateTierTree(tier);
                ++processed_tiers;
            } else {
                printf("Failed to solve tier %" PRITier ", code %d\n", tier,
                       error);
                ++failed_tiers;
            }
            time_t end = time(NULL);
            time_elapsed += difftime(end, begin);
            SolveTierTreePrintTime(tier, time_elapsed, solved, verbose);
        } else {
            ++skipped_tiers;
        }
    }
    if (verbose > 0) PrintSolverResult(time_elapsed);

    return kNoError;
}

#else  // USE_MPI

static int SolveTierTreeMpi(bool force, int verbose) {
    if (verbose > 0) {
        printf("Begin solving all %" PRId64 " tiers (%" PRId64
               " canonical) of total size %" PRId64 " (positions)\n",
               total_tiers, total_canonical_tiers, total_size);
    }

    time_t begin_time = time(NULL);
    SolveTierTreeMpiSolveAll(begin_time, force, verbose);
    SolveTierTreeMpiTerminateWorkers();
    double time_elapsed = difftime(time(NULL), begin_time);
    if (verbose > 0) PrintSolverResult(time_elapsed);

    return kNoError;
}

static void SolveTierTreeMpiTerminateWorkers(void) {
    int num_workers = SafeMpiCommSize(MPI_COMM_WORLD) - 1, num_terminated = 0;

    while (num_terminated < num_workers) {
        TierMpiWorkerMessage worker_msg;
        int worker_rank;
        TierMpiManagerRecvAnySource(&worker_msg, &worker_rank);
        TierMpiManagerSendTerminate(worker_rank);
        ++num_terminated;
    }
}

static void SolveTierTreeMpiSolveAll(time_t begin_time, bool force,
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
                SolveUpdateTierTree(tier);
                ++processed_tiers;
            }
            TierArrayRemove(&solving_tiers, tier);

            double time_elapsed = difftime(time(NULL), begin_time);
            SolveTierTreePrintTime(tier, time_elapsed, solved, verbose);
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
    current_api.GetTierName(tier_name, tier);
    printf("Dispatching tier [%s] (#%" PRITier ") to worker %d.\n", tier_name,
           tier, worker_rank);
    fflush(stdout);
}

#endif  // USE_MPI

static void SolveUpdateTierTree(Tier solved_tier) {
    TierArray parent_tiers = PopParentTiers(solved_tier);
    TierHashSet canonical_parents;
    TierHashSetInit(&canonical_parents, 0.5);
    for (int64_t i = 0; i < parent_tiers.size; ++i) {
        // Update canonical parent's number of unsolved children only.
        Tier canonical = current_api.GetCanonicalTier(parent_tiers.array[i]);
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
        if (num_unsolved_child_tiers == 1) {
            TierQueuePush(&pending_tiers, canonical);
        }
    }
    TierHashSetDestroy(&canonical_parents);
    TierArrayDestroy(&parent_tiers);
}

static void SolveTierTreePrintTime(Tier tier, double time_elapsed_seconds,
                                   bool solved, bool verbose) {
    int64_t tier_size = current_api.GetTierSize(tier);
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
        current_api.GetTierName(tier_name, tier);
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
            double time_remaining =
                time_elapsed_seconds / processed_size * remaining_size;
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
    return current_api.GetCanonicalTier(tier) == tier;
}

static int DiscoverTierTree(bool force, int verbose) {
    TierAnalyzerInit(&current_api);
    while (!TierQueueEmpty(&pending_tiers)) {
        Tier tier = TierQueuePop(&pending_tiers);
        Tier canonical = current_api.GetCanonicalTier(tier);

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
            AnalyzeUpdateTierTree(tier);
            ++processed_tiers;

            // If tier is non-canonical, we must convert the analysis to non-
            // cannonical. Note that the analysis here is the analysis of the
            // canonical tier on disk.
            if (tier != canonical) AnalysisConvertToNoncanonical(tier_analysis);
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
    current_api.GetTierName(tier_name, tier);
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

static void AnalyzeUpdateTierTree(Tier analyzed_tier) {
    TierArray child_tiers = current_api.GetChildTiers(analyzed_tier);
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
        "\n--- Finished analyzing all tiers. ---\n"
        "Number of canonical tiers analyzed: %" PRId64
        "\nNumber of tiers failed due to OOM: %" PRId64
        "\nTotal tiers scanned: %" PRId64 "\n\n",
        processed_tiers, failed_tiers, processed_tiers + failed_tiers);

    if (failed_tiers > 0) {
        printf(
            "DiscoverTierTree: (WARNING) At least one tier failed to be "
            "analyzed and the analysis of the game may be inaccurate.\n");
    }

    AnalysisPrintEverything(stdout, &game_analysis);
}

static int TestTierTree(long seed) {
    double time_elapsed = 0.0;
    printf("Begin random sanity testing of all %" PRId64 " tiers (%" PRId64
           " canonical) of total size %" PRId64 " (positions). %" PRId64
           " tiers ready in test queue\n",
           total_tiers, total_canonical_tiers, total_size,
           TierQueueSize(&pending_tiers));

    char tier_name[kDbFileNameLengthMax + 1];
    while (!TierQueueEmpty(&pending_tiers)) {
        Tier tier = TierQueuePop(&pending_tiers);
        if (IsCanonicalTier(tier)) {  // Only test canonical tiers.
            time_t begin = time(NULL);
            int error = current_api.GetTierName(tier_name, tier);
            if (error != kNoError) {
                printf("Failed to get name of tier %" PRITier "\n", tier);
                return kTierSolverTestGetTierNameError;
            }

            printf("Testing tier [%s] (#%" PRITier ") of size %" PRId64 "... ",
                   tier_name, tier, current_api.GetTierSize(tier));
            TierArray parent_tiers = GetParentTiers(tier);
            TierArrayAppend(&parent_tiers, tier);
            error = TierWorkerTest(tier, &parent_tiers, seed);
            TierArrayDestroy(&parent_tiers);
            if (error == kTierSolverTestNoError) {
                // Test passed.
                SolveUpdateTierTree(tier);
                ++processed_tiers;
            } else {
                printf("FAILED\n");
                return error;
            }
            time_t end = time(NULL);
            time_elapsed += difftime(end, begin);
            printf("PASSED. %" PRId64 " tiers ready in test queue\n",
                   TierQueueSize(&pending_tiers));
        } else {
            ++skipped_tiers;
        }
    }
    PrintTestResult(time_elapsed);

    return kTierSolverTestNoError;
}

static void PrintTestResult(double time_elapsed) {
    assert(failed_tiers == 0);
    printf(
        "Finished solving all tiers in %d second(s).\n"
        "Number of canonical tiers tested: %" PRId64
        "\nNumber of non-canonical tiers skipped: %" PRId64
        "\nTotal tiers tested: %" PRId64 "\n",
        (int)time_elapsed, processed_tiers, skipped_tiers,
        processed_tiers + skipped_tiers);
    printf("\n");
}

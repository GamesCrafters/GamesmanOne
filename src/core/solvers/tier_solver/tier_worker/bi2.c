/**
 * @file bi.c
 * @author Max Delgadillo: designed and implemented the original version
 * of the backward induction algorithm (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Separated functions for
 * solving of a single tier into its own module, implemented multithreading
 * using OpenMP, and reformatted functions for readability.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Backward induction tier worker algorithm implementation.
 * @version 1.1.5
 * @date 2025-05-27
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

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdio.h>    // printf, fprintf, stderr
#include <string.h>   // memcpy

#include "core/concurrency.h"
#include "core/constants.h"
#include "core/db/arraydb/arraydb.h"
#include "core/db/db_manager.h"
#include "core/gamesman_memory.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/solvers/tier_solver/tier_worker/bi.h"
#include "core/types/gamesman_types.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

// Note on multithreading:
//   Be careful that "if (!condition) ConcurrentBoolStore(&success, false);" is
//   not equivalent to "ConcurrentBoolStore(&success, success & condition);" or
//   "ConcurrentBoolStore(&success, condition);". The former creates a race
//   condition whereas the latter may overwrite an already failing result.

// Copy of the API functions from tier_manager. Cannot use a reference here
// because we need to create/modify some of the functions.
static TierSolverApi current_api;

// Number of positions in each database compression block.
static int64_t current_db_chunk_size;

static Tier this_tier;                // The tier being solved.
static int64_t this_tier_size;        // Size of the tier being solved.
static bool parallel_scan_this_tier;  // Whether the current tier should be
                                      // scanned in parallel.

// Array of child tiers with this_tier appended to the back.
static Tier child_tiers[kTierSolverNumChildTiersMax];
static int num_child_tiers;  // Size of child_tiers.
static int max_win_lose_remoteness;
static int max_tie_remoteness;

typedef int16_t ChildPosCounterType;
// Number of undecided child positions array (malloc'ed and owned by the
// TierWorkerSolve function). Note that we are assuming the number of children
// of ANY position is no more than 32767.
#ifdef _OPENMP
typedef _Atomic ChildPosCounterType AtomicChildPosCounterType;
static AtomicChildPosCounterType *num_undecided_children = NULL;
#else   // _OPENMP not defined
static ChildPosCounterType *num_undecided_children = NULL;
#endif  // _OPENMP

static int num_threads;  // Number of threads available.

// ------------------------------ Step0Initialize ------------------------------

static bool Step0_0SetupSolverArrays(void) {
    int error = DbManagerCreateConcurrentSolvingTier(
        this_tier, current_api.GetTierSize(this_tier));
    if (error != 0) return false;

#ifdef _OPENMP
    num_undecided_children = (AtomicChildPosCounterType *)GamesmanMalloc(
        this_tier_size * sizeof(AtomicChildPosCounterType));
    if (num_undecided_children == NULL) return false;

    for (int64_t i = 0; i < this_tier_size; ++i) {
        atomic_init(&num_undecided_children[i], 0);
    }
#else   // _OPENMP not defined
    num_undecided_children = (ChildPosCounterType *)GamesmanCallocWhole(
        this_tier_size, sizeof(ChildPosCounterType));
#endif  // _OPENMP

    return (num_undecided_children != NULL);
}

static void Step0_1SetupChildTiers(void) {
    Tier raw[kTierSolverNumChildTiersMax];
    int num_raw = current_api.GetChildTiers(this_tier, raw);
    TierHashSet dedup;
    TierHashSetInit(&dedup, 0.5);
    num_child_tiers = 0;
    for (int i = 0; i < num_raw; ++i) {
        Tier canonical = current_api.GetCanonicalTier(raw[i]);
        if (TierHashSetAdd(&dedup, canonical)) {
            child_tiers[num_child_tiers++] = canonical;
        }
    }
    TierHashSetDestroy(&dedup);
}

static bool Step0Initialize(const TierSolverApi *api, int64_t db_chunk_size,
                            Tier tier) {
    // Copy solver API function pointers and set db chunk size.
    memcpy(&current_api, api, sizeof(current_api));
    current_db_chunk_size = db_chunk_size;
    num_threads = ConcurrencyGetOmpNumThreads();

    // Initialize child tier array.
    this_tier = tier;
    this_tier_size = current_api.GetTierSize(tier);
    if (!Step0_0SetupSolverArrays()) return false;
    Step0_1SetupChildTiers();

    // From this point on, child_tiers will also contain this_tier.
    child_tiers[num_child_tiers++] = this_tier;

    // Make sure each thread gets at least one cache line of records when
    // performing a scan of the current tier to prevent false sharing.
    parallel_scan_this_tier =
        this_tier_size >=
        (int64_t)num_threads * GM_CACHE_LINE_SIZE / kArrayDbRecordSize;

    return true;
}

// -------------------------- Step1ProcessChildTiers --------------------------

static_assert(kUndecided < kLose && kLose < kDraw && kDraw < kTie &&
                  kTie < kWin,
              "The following position value relative order is required for the "
              "backward induction algorithm to work: Undecided < Lose < Draw < "
              "Tie < Win");
/**
 * @brief Returns a negative value if the value-remoteness pair (v1, r1) is
 * considered a worse outcome than (v2, r2); returns a positive value if
 * better; returns 0 if the two pairs are exactly the same.
 */
static int OutcomeCompare(Value v1, int r1, Value v2, int r2) {
    if (v1 != v2) return v1 - v2;

    // v1 == v2:
    // If the common value is kLose, return r1 - r2, otherwise return r2 - r1.
    // Reason: when losing, a larger remoteness is preferred;
    // when winning/tying, a smaller remoteness is preferred.
    return (1 - (v1 == kLose) * 2) * (r2 - r1);
}

static void DeduceParentsChildTierPosition(TierPosition child, Value value,
                                           int remoteness) {
    // Calculate parent value
    switch (value) {
        case kLose:
            value = kWin;
            ++remoteness;
            break;
        case kDraw:
            break;
        case kTie:
            ++remoteness;
            break;
        case kWin:
            value = kLose;
            ++remoteness;
            break;
        default:
            return;
    }

    Position parents[kTierSolverNumParentPositionsMax];
    int num_parents =
        current_api.GetCanonicalParentPositions(child, this_tier, parents);
    for (int i = 0; i < num_parents; ++i) {
        DbManagerMaximizeValueRemoteness(parents[i], value, remoteness,
                                         OutcomeCompare);
    }
}

static void UpdateMaxRemotenesses(Value val, int remoteness,
                                  ConcurrentInt *max_wl,
                                  ConcurrentInt *max_tie) {
    switch (val) {
        case kLose:
        case kWin:
            ConcurrentIntMax(max_wl, remoteness);
            break;
        case kTie:
            ConcurrentIntMax(max_tie, remoteness);
            break;
        default:
            return;
    }
}

static void ProcessChildTier(int child_index, ConcurrentInt *max_wl,
                             ConcurrentInt *max_tie) {
    Tier child_tier = child_tiers[child_index];

    // Scan child tier maximize their parents' values.
    int64_t child_tier_size = current_api.GetTierSize(child_tier);
    PRAGMA_OMP(parallel if (child_tier_size > current_db_chunk_size)) {
        DbProbe probe;
        DbManagerProbeInit(&probe);
        TierPosition child = {.tier = child_tier};
        PRAGMA_OMP(for schedule(dynamic, current_db_chunk_size))
        for (Position position = 0; position < child_tier_size; ++position) {
            child.position = position;
            Value value = DbManagerProbeValue(&probe, child);
            if (value == kUndecided) continue;
            int remoteness = DbManagerProbeRemoteness(&probe, child);
            UpdateMaxRemotenesses(value, remoteness, max_wl, max_tie);
            DeduceParentsChildTierPosition(child, value, remoteness);
        }
        DbManagerProbeDestroy(&probe);
    }
}

/**
 * @brief Load all non-drawing positions from all child tiers into frontier.
 */
static void Step1ProcessChildTiers(void) {
    ConcurrentInt concurrent_max_win_lose_rmt, concurrent_max_tie_rmt;
    ConcurrentIntInit(&concurrent_max_win_lose_rmt, 0);
    ConcurrentIntInit(&concurrent_max_tie_rmt, 0);

    // -1 because this_tier is the last element in child_tiers.
    for (int child_index = 0; child_index < num_child_tiers - 1;
         ++child_index) {
        // Load child tier from disk.
        ProcessChildTier(child_index, &concurrent_max_win_lose_rmt,
                         &concurrent_max_tie_rmt);
    }

    max_win_lose_remoteness = ConcurrentIntLoad(&concurrent_max_win_lose_rmt);
    max_tie_remoteness = ConcurrentIntLoad(&concurrent_max_tie_rmt);
}

// ------------------------------- Step2ScanTier -------------------------------

static bool IsCanonicalPosition(TierPosition tp) {
    return current_api.GetCanonicalPosition(tp) == tp.position;
}

static void SetNumUndecidedChildren(Position pos, ChildPosCounterType value) {
#ifdef _OPENMP
    atomic_store_explicit(&num_undecided_children[pos], value,
                          memory_order_relaxed);
#else   // _OPENMP not defined
    num_undecided_children[pos] = value;
#endif  // _OPENMP
}

static ChildPosCounterType GetNumberOfCanonicalChildPositionsInThisTier(
    TierPosition tp) {
    TierPosition children[kTierSolverNumChildPositionsMax];
    int num_children = current_api.GetCanonicalChildPositions(tp, children);
    ChildPosCounterType ret = 0;
    for (int i = 0; i < num_children; ++i) {
        ret += (children[i].tier == this_tier);
    }

    return ret;
}

/**
 * @brief Scans the current tier for illegal and primitive positions and counts
 * the number of child positions within the same tier.
 */
static void Step2ScanTier(void) {
    PRAGMA_OMP(parallel if (parallel_scan_this_tier)) {
        PRAGMA_OMP(for schedule(dynamic, 128))
        for (Position position = 0; position < this_tier_size; ++position) {
            TierPosition tp = {.tier = this_tier, .position = position};

            // Skip illegal positions and non-canonical positions.
            if (!current_api.IsLegalPosition(tp) || !IsCanonicalPosition(tp)) {
                continue;
            }

            Value value = current_api.Primitive(tp);
            if (value != kUndecided) {  // tp is a primitive position
                DbManagerSetValueRemoteness(position, value, 0);
            } else {  // tp is not a primitive position
                SetNumUndecidedChildren(
                    position, GetNumberOfCanonicalChildPositionsInThisTier(tp));
            }
        }
    }
}

// ---------------------------- Step3PushFrontierUp ----------------------------

static bool DeduceParentsFromWinningChild(Position pos, int child_rmt) {
    TierPosition tp = {.tier = this_tier, .position = pos};
    Position parents[kTierSolverNumParentPositionsMax];
    int num_parents =
        current_api.GetCanonicalParentPositions(tp, this_tier, parents);
    bool advance = false;
    for (int i = 0; i < num_parents; ++i) {
#ifdef _OPENMP
        ChildPosCounterType child_remaining = atomic_fetch_sub_explicit(
            &num_undecided_children[parents[i]], 1, memory_order_relaxed);
#else   // _OPENMP not defined
        ChildPosCounterType child_remaining =
            num_undecided_children[parents[i]]--;
#endif  // _OPENMP
        // If this child position is the last undecided child of the parent
        // position, the parent is losing in (child_rmt + 1), unless there is
        // a way to do better by moving to one of the child tiers.
        if (child_remaining == 1) {
            advance |= DbManagerMaximizeValueRemoteness(
                parents[i], kLose, child_rmt + 1, OutcomeCompare);
        }
    }

    return advance;
}

static bool DeduceParentsFromLosingOrTyingChild(Position pos, int child_rmt,
                                                Value parent_val) {
    TierPosition tp = {.tier = this_tier, .position = pos};
    Position parents[kTierSolverNumParentPositionsMax];
    int num_parents =
        current_api.GetCanonicalParentPositions(tp, this_tier, parents);
    bool advance = false;
    for (int i = 0; i < num_parents; ++i) {
#ifdef _OPENMP
        // Atomically fetch num_undecided_children[parents[i]] into
        // child_remaining and set it to zero.
        ChildPosCounterType child_remaining = atomic_exchange_explicit(
            &num_undecided_children[parents[i]], 0, memory_order_relaxed);
#else                                        // _OPENMP not defined
        ChildPosCounterType child_remaining =
            num_undecided_children[parents[i]];
        num_undecided_children[parents[i]] = 0;
#endif                                       // _OPENMP
        if (child_remaining <= 0) continue;  // Parent already solved.

        // All parents are winning/tying in (child_rmt + 1). It is not possible
        // that they can win faster by moving to a child tier because otherwise
        // they would have been processed earlier in Step3_0PushWinLose.
        DbManagerSetValueRemoteness(parents[i], parent_val, child_rmt + 1);
        advance = true;
    }

    return advance;
}

static ChildPosCounterType GetNumUndecidedChildren(Position pos) {
#ifdef _OPENMP
    return atomic_load_explicit(&num_undecided_children[pos],
                                memory_order_relaxed);
#else   // _OPENMP not defined
    return num_undecided_children[pos];
#endif  // _OPENMP
}

static bool Step3_0PushWinLose(int child_rmt) {
    bool advance = false;

    // Scan the current tier for positions that are solved in the previous scan.
    PRAGMA_OMP(parallel if (parallel_scan_this_tier)) {
        PRAGMA_OMP(for schedule(dynamic, 128) reduction(||:advance))
        for (Position position = 0; position < this_tier_size; ++position) {
            Value value = DbManagerGetValue(position);
            if (value != kWin && value != kLose) continue;
            int rmt = DbManagerGetRemoteness(position);
            if (rmt != child_rmt) continue;

            bool local_advance;
            if (value == kWin) {
                SetNumUndecidedChildren(position, 0);
                local_advance =
                    DeduceParentsFromWinningChild(position, child_rmt);
            } else {
                if (GetNumUndecidedChildren(position) > 0) continue;
                local_advance = DeduceParentsFromLosingOrTyingChild(
                    position, child_rmt, kWin);
            }
            if (local_advance) advance = true;
        }
    }

    return advance;
}

static bool Step3_1PushTie(int child_rmt) {
    bool advance = false;

    // Scan the current tier for positions that are solved in the previous scan.
    PRAGMA_OMP(parallel if (parallel_scan_this_tier)) {
        PRAGMA_OMP(for schedule(dynamic, 128) reduction(||:advance))
        for (Position position = 0; position < this_tier_size; ++position) {
            Value value = DbManagerGetValue(position);
            if (value != kTie) continue;
            int rmt = DbManagerGetRemoteness(position);
            if (rmt != child_rmt) continue;

            SetNumUndecidedChildren(position, 0);
            bool local_advance =
                DeduceParentsFromLosingOrTyingChild(position, child_rmt, kTie);
            if (local_advance) advance = true;
        }
    }

    return advance;
}

/**
 * @brief Pushes frontier up.
 */
static void Step3PushFrontierUp(void) {
    // Process winning and losing positions first. Remotenesses must be
    // processed in ascending order.
    int remoteness = 0;
    bool advance = true;
    while (remoteness <= max_win_lose_remoteness || advance) {
        advance = Step3_0PushWinLose(remoteness++);
    }

    // Then move on to tying positions.
    remoteness = 0;
    advance = true;
    while (remoteness <= max_tie_remoteness || advance) {
        advance = Step3_1PushTie(remoteness++);
    }
}

// -------------------------- Step4MarkDrawPositions --------------------------

static void Step4MarkDrawPositions(void) {
    PRAGMA_OMP(parallel for if(parallel_scan_this_tier))
    for (Position position = 0; position < this_tier_size; ++position) {
        if (GetNumUndecidedChildren(position) > 0) {
            // There exists a way to draw the game at a position if it still has
            // undecided children.
            DbManagerMaximizeValueRemoteness(position, kDraw, 0,
                                             OutcomeCompare);
            continue;
        }
    }
    GamesmanFree(num_undecided_children);
    num_undecided_children = NULL;
}

// ------------------------------ Step5SaveValues ------------------------------

static void Step5SaveValues(void) {
    if (DbManagerFlushSolvingTier(NULL) != 0) {
        fprintf(stderr,
                "Step5SaveValues: an error has occurred while flushing of the "
                "current tier. The database file for tier %" PRITier
                " may be corrupt.\n",
                this_tier);
    }
    if (DbManagerFreeSolvingTier() != 0) {
        fprintf(stderr,
                "Step5SaveValues: an error has occurred while freeing of the "
                "current tier's in-memory database. Tier: %" PRITier "\n",
                this_tier);
    }
}

// --------------------------------- CompareDb ---------------------------------

static bool CompareDb(void) {
    DbProbe probe, ref_probe;
    if (DbManagerProbeInit(&probe)) return false;
    if (DbManagerRefProbeInit(&ref_probe)) {
        DbManagerProbeDestroy(&probe);
        return false;
    }

    bool success = true;
    for (Position p = 0; p < this_tier_size; ++p) {
        TierPosition tp = {.tier = this_tier, .position = p};
        Value ref_value = DbManagerRefProbeValue(&ref_probe, tp);
        if (ref_value == kUndecided) continue;

        Value actual_value = DbManagerProbeValue(&probe, tp);
        if (actual_value != ref_value) {
            printf("CompareDb: inconsistent value at tier %" PRITier
                   " position %" PRIPos "\n",
                   this_tier, p);
            success = false;
            goto _bailout;
        }

        int actual_remoteness = DbManagerProbeRemoteness(&probe, tp);
        int ref_remoteness = DbManagerRefProbeRemoteness(&ref_probe, tp);
        if (actual_remoteness != ref_remoteness) {
            printf("CompareDb: inconsistent remoteness at tier %" PRITier
                   " position %" PRIPos "\n",
                   this_tier, p);
            success = false;
            goto _bailout;
        }
    }

_bailout:
    DbManagerProbeDestroy(&probe);
    DbManagerRefProbeDestroy(&ref_probe);
    if (success) {
        printf("CompareDb: tier %" PRITier " check passed\n", this_tier);
    }

    return success;
}

// ------------------------------- Step6Cleanup -------------------------------

static void Step6Cleanup(void) {
    this_tier = kIllegalTier;
    this_tier_size = kIllegalSize;
    parallel_scan_this_tier = false;
    num_child_tiers = 0;
    DbManagerFreeSolvingTier();
    GamesmanFree(num_undecided_children);
    num_undecided_children = NULL;
    num_threads = 0;
}

// -----------------------------------------------------------------------------
// ------------------------- TierWorkerSolveBIInternal -------------------------
// -----------------------------------------------------------------------------

int TierWorkerSolveBIInternal(const TierSolverApi *api, int64_t db_chunk_size,
                              Tier tier, const TierWorkerSolveOptions *options,
                              bool *solved) {
    if (solved != NULL) *solved = false;
    int ret = kRuntimeError;
    if (!options->force && DbManagerTierStatus(tier) == kDbTierStatusSolved) {
        ret = kNoError;  // Success.
        goto _bailout;
    }

    /* Solver main algorithm. */
    if (!Step0Initialize(api, db_chunk_size, tier)) goto _bailout;
    Step1ProcessChildTiers();
    Step2ScanTier();
    Step3PushFrontierUp();
    Step4MarkDrawPositions();
    Step5SaveValues();
    if (options->compare && !CompareDb()) goto _bailout;
    if (solved != NULL) *solved = true;
    ret = kNoError;  // Success.

_bailout:
    Step6Cleanup();
    return ret;
}

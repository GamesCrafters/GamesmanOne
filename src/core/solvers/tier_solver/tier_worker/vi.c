/**
 * @file vi.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Value iteration tier worker algorithm.
 * @version 1.1.0
 * @date 2024-11-14
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

#include "core/solvers/tier_solver/tier_worker/vi.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stdint.h>   // int32_t, int64_t
#include <stdio.h>    // puts, printf, fprintf, stderr

#include "core/concurrency.h"
#include "core/constants.h"
#include "core/db/db_manager.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"
#include "libs/lz4_utils/lz4_utils.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

// ----------------------------------- Types -----------------------------------

enum ValueIterationSteps {
    kNotStarted,
    kScanningTier,
    kIteratingWinLose,
    kIteratingTie,
    kMarkingDraw,
};

typedef struct {
    int32_t step;
    int32_t remoteness;
} CheckpointStatus;

// ----------------------------- Global Variables -----------------------------

// Note on multithreading:
//   Be careful that "if (!condition) success = false;" is not equivalent to
//   "success &= condition" or "success = condition". The former creates a race
//   condition whereas the latter may overwrite an already failing result.

// Reference to the set of tier solver API functions for the current game.
static const TierSolverApi *api_internal;

// Level of verbosity.
static int verbose;

static Tier this_tier;          // The tier being solved.
static int64_t this_tier_size;  // Size of the tier being solved.

// Child tiers of the tier being solved.
static Tier child_tiers[kTierSolverNumChildTiersMax];
static int num_child_tiers;  // Number of child tiers.

// The maximum remoteness discovered at any winning/losing positions in the
// child tiers of this tier.
static int max_win_lose_remoteness;

// The maximum remoteness discovered at any tying positions in the child tiers
// of this tier.
static int max_tie_remoteness;

// Last checkpoint time.
static time_t prev_checkpoint;

// Time cost to save the previous checkpoint in seconds. Updated every
// checkpoint.
static double checkpoint_save_cost;

// ------------------------------ Step0Initialize ------------------------------

static bool Step0_0SetupChildTiers(void) {
    Tier raw[kTierSolverNumChildTiersMax];
    int num_raw = api_internal->GetChildTiers(this_tier, raw);
    TierHashSet dedup;
    TierHashSetInit(&dedup, 0.5);
    num_child_tiers = 0;
    for (int i = 0; i < num_raw; ++i) {
        Tier canonical = api_internal->GetCanonicalTier(raw[i]);

        // Another child tier is symmetric to this one and was already added.
        if (TierHashSetContains(&dedup, canonical)) continue;

        TierHashSetAdd(&dedup, canonical);
        child_tiers[num_child_tiers++] = canonical;
    }
    TierHashSetDestroy(&dedup);

    return true;
}

// Typically returns an overestimated result.
static double GetCheckpointSaveCostEstimate(void) {
    static const double kOverhead = 1;
    static const double kTypicalHDDSpeed = 200 << 20;  // 200 MiB/s

    return kOverhead +
           (double)DbManagerTierMemUsage(this_tier, this_tier_size) /
               kTypicalHDDSpeed;
}

static bool Step0Initialize(const TierSolverApi *api, Tier tier,
                            int verbosity) {
    api_internal = api;
    this_tier = tier;
    verbose = verbosity;
    if (!Step0_0SetupChildTiers()) return false;

    this_tier_size = api_internal->GetTierSize(tier);
    max_win_lose_remoteness = 0;
    max_tie_remoteness = 0;
    checkpoint_save_cost = GetCheckpointSaveCostEstimate();

    return true;
}

// ----------------------------- Step1LoadChildren -----------------------------

static bool Step1LoadChildren(void) {
    for (int i = 0; i < num_child_tiers; ++i) {
        Tier child_tier = child_tiers[i];
        int64_t size = api_internal->GetTierSize(child_tier);
        int error = DbManagerLoadTier(child_tier, size);
        if (error != kNoError) return false;

        // Scan for largest remotenesses
        PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(16)
        for (Position pos = 0; pos < size; ++pos) {
            Value val = DbManagerGetValueFromLoaded(child_tier, pos);
            switch (val) {
                case kWin:
                case kLose: {
                    int r = DbManagerGetRemotenessFromLoaded(child_tier, pos);
                    if (r > max_win_lose_remoteness) {
                        max_win_lose_remoteness = r;
                    }
                    break;
                }

                case kTie: {
                    int r = DbManagerGetRemotenessFromLoaded(child_tier, pos);
                    if (r > max_tie_remoteness) {
                        max_tie_remoteness = r;
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }

    return true;
}

// --------------------------- Step2SetupSolvingTier ---------------------------

/**
 * @brief Loads a checkpoint and its metadata into \p ct if exists, or creates a
 * new solving tier and leave \p ct unmodified otherwise.
 */
static bool Step2SetupSolvingTier(CheckpointStatus *ct) {
    if (DbManagerCheckpointExists(this_tier)) {
        if (verbose > 1) PrintfAndFlush("Loading checkpoint...");
        int error =
            DbManagerCheckpointLoad(this_tier, this_tier_size, ct, sizeof(*ct));
        if (verbose > 1) puts(error == kNoError ? "done" : "failed");
        return (error == kNoError);
    }

    int error = DbManagerCreateSolvingTier(this_tier, this_tier_size);
    if (error != kNoError) return false;

    return true;
}

// ------------------------------- Step3ScanTier -------------------------------

static bool IsCanonicalPosition(Position position) {
    TierPosition tier_position = {.tier = this_tier, .position = position};
    return api_internal->GetCanonicalPosition(tier_position) == position;
}

static void Step3ScanTier(void) {
    if (verbose > 1) PrintfAndFlush("Value iteration: scanning tier... ");
    PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(256)
    for (Position pos = 0; pos < this_tier_size; ++pos) {
        TierPosition tier_position = {.tier = this_tier, .position = pos};
        if (!api_internal->IsLegalPosition(tier_position) ||
            !IsCanonicalPosition(pos)) {
            // Temporarily mark illegal and non-canonical positions as
            // drawing. These values will be changed to undecided later.
            DbManagerSetValue(pos, kDraw);
            continue;
        }

        Value value = api_internal->Primitive(tier_position);
        if (value != kUndecided) {  // If tier_position is primitive...
            // Set its value immediately.
            DbManagerSetValue(pos, value);
            DbManagerSetRemoteness(pos, 0);
        }  // Otherwise, do nothing.
    }
    if (verbose > 1) puts("done");
}

// ------------------------------- Step4Iterate -------------------------------

static bool IterateWinLoseProcessPosition(int iteration, Position pos,
                                          bool *updated) {
    *updated = false;
    bool all_children_winning = true;
    int largest_win = -1;
    TierPosition tier_position = {.tier = this_tier, .position = pos};
    TierPosition child_positions[kTierSolverNumChildPositionsMax];
    int num_child_positions = api_internal->GetCanonicalChildPositions(
        tier_position, child_positions);
    for (int i = 0; i < num_child_positions; ++i) {
        TierPosition child_tier_position = child_positions[i];
        Value child_value;
        int child_remoteness;
        if (child_tier_position.tier == this_tier) {
            child_value = DbManagerGetValue(child_tier_position.position);
            child_remoteness =
                DbManagerGetRemoteness(child_tier_position.position);
        } else {
            child_value = DbManagerGetValueFromLoaded(
                child_tier_position.tier, child_tier_position.position);
            child_remoteness = DbManagerGetRemotenessFromLoaded(
                child_tier_position.tier, child_tier_position.position);
        }
        switch (child_value) {
            case kUndecided:
            case kTie:
            case kDraw:
                all_children_winning = false;
                break;
            case kLose:
                all_children_winning = false;
                if (child_remoteness == iteration - 1) {
                    DbManagerSetValue(pos, kWin);
                    DbManagerSetRemoteness(pos, iteration);
                    *updated = true;
                    return true;
                }
                break;
            case kWin:
                if (child_remoteness > largest_win) {
                    largest_win = child_remoteness;
                }
                break;
            default:
                fprintf(stderr,
                        "IterateWinLoseProcessPosition: unknown value %d\n",
                        child_value);
                break;
        }
    }

    if (all_children_winning && largest_win + 1 == iteration) {
        DbManagerSetValue(pos, kLose);
        DbManagerSetRemoteness(pos, iteration);
        *updated = true;
    }

    return true;
}

static bool CheckpointNeeded(time_t prev, time_t curr) {
    // Suppose it takes the same amount of time to save and load the same
    // checkpoint. If it takes less time to save and load a checkpoint than it
    // does to redo what was done since the previous checkpoint, then it is
    // worth saving a new checkpoint.
    return (difftime(curr, prev) > checkpoint_save_cost * 2.0);
}

static int CheckpointSave(int step, int remoteness) {
    clock_t begin = clock();
    CheckpointStatus ct = {.step = step, .remoteness = remoteness};
    int ret = DbManagerCheckpointSave(&ct, sizeof(ct));
    checkpoint_save_cost = ((double)(clock() - begin)) / CLOCKS_PER_SEC;

    return ret;
}

static bool Step4_0IterateWinLose(int initial_remoteness) {
    ConcurrentBool updated, failed;
    ConcurrentBoolInit(&updated, true);
    ConcurrentBoolInit(&failed, false);

    int i = initial_remoteness;
    if (verbose > 1) {
        PrintfAndFlush("Value iteration: begin iterations for W/L positions");
        for (i = 1; i < initial_remoteness; ++i) {
            printf(".");  // Restore previous progress bar from checkpoint.
        }
        fflush(stdout);
    }

    while (ConcurrentBoolLoad(&updated) || i <= max_win_lose_remoteness + 1) {
        // Save a checkpoint if needed.
        bool checkpoint = CheckpointNeeded(prev_checkpoint, time(NULL));
        if (verbose > 1) PrintfAndFlush(checkpoint ? "," : ".");
        if (checkpoint) {
            if (CheckpointSave(kIteratingWinLose, i) != kNoError) return false;
            prev_checkpoint = time(NULL);
        }

        ConcurrentBoolStore(&updated, false);
        PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(128)
        for (Position pos = 0; pos < this_tier_size; ++pos) {
            bool pos_updated;
            if (DbManagerGetValue(pos) != kUndecided) continue;
            bool success = IterateWinLoseProcessPosition(i, pos, &pos_updated);
            if (!success) ConcurrentBoolStore(&failed, true);
            if (pos_updated) ConcurrentBoolStore(&updated, true);
        }

        if (ConcurrentBoolLoad(&failed)) return false;
        ++i;
    }
    if (verbose > 1) puts("done");

    return true;
}

static bool IterateTieProcessPosition(int iteration, Position pos,
                                      bool *updated) {
    *updated = false;
    TierPosition tier_position = {.tier = this_tier, .position = pos};
    TierPosition child_positions[kTierSolverNumChildPositionsMax];
    int num_child_positions = api_internal->GetCanonicalChildPositions(
        tier_position, child_positions);
    for (int64_t i = 0; i < num_child_positions; ++i) {
        TierPosition child_tier_position = child_positions[i];
        Value child_value;
        int child_remoteness;
        if (child_tier_position.tier == this_tier) {
            child_value = DbManagerGetValue(child_tier_position.position);
            child_remoteness =
                DbManagerGetRemoteness(child_tier_position.position);
        } else {
            child_value = DbManagerGetValueFromLoaded(
                child_tier_position.tier, child_tier_position.position);
            child_remoteness = DbManagerGetRemotenessFromLoaded(
                child_tier_position.tier, child_tier_position.position);
        }
        if (child_value == kTie && child_remoteness == iteration - 1) {
            DbManagerSetValue(pos, kTie);
            DbManagerSetRemoteness(pos, iteration);
            *updated = true;
            break;
        }
    }

    return true;
}

static bool Step4_1IterateTie(int initial_remoteness) {
    ConcurrentBool updated, failed;
    ConcurrentBoolInit(&updated, true);
    ConcurrentBoolInit(&failed, false);

    int i = initial_remoteness;
    if (verbose > 1) {
        PrintfAndFlush("Value iteration: begin iterations for T positions");
        for (i = 1; i < initial_remoteness; ++i) {
            printf(".");  // Restore previous progress from checkpoint.
        }
        fflush(stdout);
    }

    while (ConcurrentBoolLoad(&updated) || i <= max_tie_remoteness + 1) {
        // Save a checkpoint if needed.
        bool checkpoint = CheckpointNeeded(prev_checkpoint, time(NULL));
        if (verbose > 1) PrintfAndFlush(checkpoint ? "," : ".");
        if (checkpoint) {
            if (CheckpointSave(kIteratingTie, i) != kNoError) return false;
            prev_checkpoint = time(NULL);
        }

        ConcurrentBoolStore(&updated, false);
        PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(256)
        for (Position pos = 0; pos < this_tier_size; ++pos) {
            bool pos_updated;
            if (DbManagerGetValue(pos) != kUndecided) continue;
            bool success = IterateTieProcessPosition(i, pos, &pos_updated);
            if (!success) ConcurrentBoolStore(&failed, true);
            if (pos_updated) ConcurrentBoolStore(&updated, true);
        }

        if (ConcurrentBoolLoad(&failed)) return false;
        ++i;
    }
    if (verbose > 1) puts("done");

    return true;
}

static bool Step4Iterate(int step, int remoteness) {
    bool success = true;
    if (step <= kIteratingWinLose) {
        success =
            Step4_0IterateWinLose(step == kIteratingWinLose ? remoteness : 1);
        if (!success) return false;
    }

    if (step <= kIteratingTie) {
        success = Step4_1IterateTie(step == kIteratingTie ? remoteness : 1);
        if (!success) return false;
    }

    // The child tiers are no longer needed.
    for (int i = 0; i < num_child_tiers; ++i) {
        DbManagerUnloadTier(child_tiers[i]);
    }

    return true;
}

// -------------------------- Step5MarkDrawPositions --------------------------

static bool Step5MarkDrawPositions(void) {
    // Save a checkpoint if needed.
    if (CheckpointNeeded(prev_checkpoint, time(NULL))) {
        if (CheckpointSave(kMarkingDraw, 0) != kNoError) return false;
        prev_checkpoint = time(NULL);
    }

    if (verbose > 1) {
        PrintfAndFlush("Value iteration: begin marking D positions... ");
    }

    PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(256)
    for (Position pos = 0; pos < this_tier_size; ++pos) {
        Value val = DbManagerGetValue(pos);
        if (val == kUndecided) {
            DbManagerSetValue(pos, kDraw);
        } else if (val == kDraw) {
            DbManagerSetValue(pos, kUndecided);
        }
    }
    if (verbose > 1) puts("done");

    return true;
}

// ------------------------------- Step6FlushDb -------------------------------

static void Step6FlushDb(void) {
    if (verbose > 1) PrintfAndFlush("Value iteration: flusing DB... ");
    if (DbManagerFlushSolvingTier(NULL) != 0) {
        fprintf(stderr,
                "Step6FlushDb: an error has occurred while flushing of the "
                "current tier. The database file for tier %" PRITier
                " may be corrupt.\n",
                this_tier);
    }
    if (DbManagerFreeSolvingTier() != 0) {
        fprintf(stderr,
                "Step6FlushDb: an error has occurred while freeing of the "
                "current tier's in-memory database. Tier: %" PRITier "\n",
                this_tier);
    }
    if (verbose > 1) puts("done");
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

// ------------------------------- Step7Cleanup -------------------------------

static bool Step7Cleanup(void) {
    int error = kNoError;
    if (DbManagerCheckpointExists(this_tier)) {
        error = DbManagerCheckpointRemove(this_tier);
    }
    this_tier = kIllegalTier;
    this_tier_size = kIllegalSize;
    for (int64_t i = 0; i < num_child_tiers; ++i) {
        Tier child_tier = child_tiers[i];
        if (DbManagerIsTierLoaded(child_tier)) DbManagerUnloadTier(child_tier);
    }
    num_child_tiers = 0;
    DbManagerFreeSolvingTier();

    return error == kNoError;
}

// -----------------------------------------------------------------------------
// ------------------------- TierWorkerSolveVIInternal -------------------------
// -----------------------------------------------------------------------------

int TierWorkerSolveVIInternal(const TierSolverApi *api, Tier tier,
                              const TierWorkerSolveOptions *options,
                              bool *solved) {
    if (solved != NULL) *solved = false;
    int ret = kRuntimeError;
    if (!options->force && DbManagerTierStatus(tier) == kDbTierStatusSolved) {
        ret = kNoError;  // Success.
        goto _bailout;
    }

    /* Value Iteration main algorithm. */
    if (!Step0Initialize(api, tier, options->verbose)) goto _bailout;
    if (!Step1LoadChildren()) goto _bailout;
    CheckpointStatus ct = {.step = kNotStarted, .remoteness = -1};
    if (!Step2SetupSolvingTier(&ct)) goto _bailout;

    prev_checkpoint = time(NULL);  // Enable checkpoints from here.
    if (ct.step <= kScanningTier) Step3ScanTier();
    if (ct.step <= kIteratingTie && !Step4Iterate(ct.step, ct.remoteness)) {
        goto _bailout;
    }
    if (!Step5MarkDrawPositions()) goto _bailout;
    Step6FlushDb();
    if (options->compare && !CompareDb()) goto _bailout;
    if (solved != NULL) *solved = true;
    ret = kNoError;  // Success.

_bailout:
    if (!Step7Cleanup()) {
        fprintf(stdout,
                "TierWorkerSolveVIInternal: bug detected at cleanup step\n");
    }
    return ret;
}

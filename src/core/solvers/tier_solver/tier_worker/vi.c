/**
 * @file vi.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Value iteration tier worker algorithm.
 * @version 1.0.0
 * @date 2024-07-11
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

#include <stdbool.h>  // bool, true, false
#include <stdio.h>    // fprintf, stderr

#include "core/constants.h"
#include "core/db/db_manager.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>        // OpenMP pragmas
#include <stdatomic.h>  // atomic_uchar, atomic functions, memory_order_relaxed
#define PRAGMA(X) _Pragma(#X)
#define PRAGMA_OMP_PARALLEL PRAGMA(omp parallel)
#define PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(k) PRAGMA(omp for schedule(dynamic, k))
#define PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(k) PRAGMA(omp parallel for schedule(dynamic, k))

#else  // _OPENMP not defined, the following macros do nothing.
#define PRAGMA
#define PRAGMA_OMP_PARALLEL
#define PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(k)
#define PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(k)
#endif  // _OPENMP

// Note on multithreading:
//   Be careful that "if (!condition) success = false;" is not equivalent to
//   "success &= condition" or "success = condition". The former creates a race
//   condition whereas the latter may overwrite an already failing result.

// Reference to the set of tier solver API functions for the current game.
static const TierSolverApi *api_internal;

static Tier this_tier;          // The tier being solved.
static int64_t this_tier_size;  // Size of the tier being solved.
// Array of child tiers with this_tier appended to the back.
static TierArray child_tiers;

static int largest_win_lose_remoteness;
static int largest_tie_remoteness;

// ------------------------------ Step0Initialize ------------------------------

static bool Step0Initialize(const TierSolverApi *api, Tier tier) {
    api_internal = api;
    this_tier = tier;
    child_tiers = api_internal->GetChildTiers(this_tier);
    if (child_tiers.size == kIllegalSize) return false;

    this_tier_size = api_internal->GetTierSize(tier);
    largest_win_lose_remoteness = 0;
    largest_tie_remoteness = 0;
    return true;
}

// ----------------------------- Step1LoadChildren -----------------------------

static bool Step1LoadChildren(void) {
    bool success = true;
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        Tier child_tier = child_tiers.array[i];
        int64_t size = api_internal->GetTierSize(child_tier);
        int error = DbManagerLoadTier(child_tier, size);
        if (error != kNoError) {
            success = false;
            break;
        }

        // Scan for largest remotenesses
        PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(16)
        for (Position pos = 0; pos < size; ++pos) {
            Value val = DbManagerGetValueFromLoaded(child_tier, pos);
            switch (val) {
                case kWin:
                case kLose: {
                    int r = DbManagerGetRemotenessFromLoaded(child_tier, pos);
                    if (r > largest_win_lose_remoteness) {
                        largest_win_lose_remoteness = r;
                    }
                    break;
                }

                case kTie: {
                    int r = DbManagerGetRemotenessFromLoaded(child_tier, pos);
                    if (r > largest_tie_remoteness) {
                        largest_tie_remoteness = r;
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }

    return success;
}

// --------------------------- Step2SetupSolvingTier ---------------------------

static bool Step2SetupSolvingTier(void) {
    int error = DbManagerCreateSolvingTier(this_tier, this_tier_size);
    if (error != kNoError) return false;

    return true;
}

static bool IsCanonicalPosition(Position position) {
    TierPosition tier_position = {.tier = this_tier, .position = position};
    return api_internal->GetCanonicalPosition(tier_position) == position;
}

// ------------------------------- Step3ScanTier -------------------------------

static void Step3ScanTier(void) {
    PRAGMA_OMP_PARALLEL {
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(256)
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
    }
}

// ------------------------------- Step4Iterate -------------------------------

static bool IterateWinLoseProcessPosition(int iteration, Position pos,
                                          bool *updated) {
    *updated = false;
    bool all_children_winning = true;
    int largest_win = -1;
    TierPosition tier_position = {.tier = this_tier, .position = pos};
    TierPositionArray child_positions =
        api_internal->GetCanonicalChildPositions(tier_position);
    if (child_positions.size == kIllegalSize) return false;

    for (int64_t i = 0; i < child_positions.size; ++i) {
        TierPosition child_tier_position = child_positions.array[i];
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
                    TierPositionArrayDestroy(&child_positions);
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

    TierPositionArrayDestroy(&child_positions);
    return true;
}

static bool Step4_0IterateWinLose(void) {
    bool updated = true;
    bool failed = false;

    for (int i = 1; updated || i <= largest_win_lose_remoteness + 1; ++i) {
        updated = false;
        PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(128)
        for (Position pos = 0; pos < this_tier_size; ++pos) {
            bool pos_updated;
            if (DbManagerGetValue(pos) != kUndecided) continue;
            bool success = IterateWinLoseProcessPosition(i, pos, &pos_updated);
            if (!success) failed = true;
            if (pos_updated) updated = true;
        }

        if (failed) return false;
    }

    return true;
}

static bool IterateTieProcessPosition(int iteration, Position pos,
                                      bool *updated) {
    *updated = false;
    TierPosition tier_position = {.tier = this_tier, .position = pos};
    TierPositionArray child_positions =
        api_internal->GetCanonicalChildPositions(tier_position);
    if (child_positions.size == kIllegalSize) return false;

    for (int64_t i = 0; i < child_positions.size; ++i) {
        TierPosition child_tier_position = child_positions.array[i];
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

    TierPositionArrayDestroy(&child_positions);
    return true;
}

static bool Step4_1IterateTie(void) {
    bool updated = false;
    bool failed = false;

    for (int i = 1; updated || i <= largest_tie_remoteness + 1; ++i) {
        updated = false;
        PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(256)
        for (Position pos = 0; pos < this_tier_size; ++pos) {
            bool pos_updated;
            if (DbManagerGetValue(pos) != kUndecided) continue;
            bool success = IterateTieProcessPosition(i, pos, &pos_updated);
            if (!success) failed = true;
            if (pos_updated) updated = true;
        }

        if (failed) return false;
    }

    return true;
}

static bool Step4Iterate(void) {
    bool success = Step4_0IterateWinLose();
    if (!success) return false;

    success = Step4_1IterateTie();
    if (!success) return false;

    // The child tiers are no longer needed.
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        DbManagerUnloadTier(child_tiers.array[i]);
    }

    return true;
}

// -------------------------- Step5MarkDrawPositions --------------------------

static void Step5MarkDrawPositions(void) {
    PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(256)
    for (Position pos = 0; pos < this_tier_size; ++pos) {
        Value val = DbManagerGetValue(pos);
        if (val == kUndecided) {
            DbManagerSetValue(pos, kDraw);
        } else if (val == kDraw) {
            DbManagerSetValue(pos, kUndecided);
        }
    }
}

// ------------------------------- Step6FlushDb -------------------------------

static void Step6FlushDb(void) {
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
    if (success)
        printf("CompareDb: tier %" PRITier " check passed\n", this_tier);

    return success;
}

// ------------------------------- Step7Cleanup -------------------------------

static void Step7Cleanup(void) {
    this_tier = kIllegalTier;
    this_tier_size = kIllegalSize;
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        Tier child_tier = child_tiers.array[i];
        if (DbManagerIsTierLoaded(child_tier)) DbManagerUnloadTier(child_tier);
    }
    TierArrayDestroy(&child_tiers);
    DbManagerFreeSolvingTier();
}

// -----------------------------------------------------------------------------
// ------------------------- TierWorkerSolveVIInternal -------------------------
// -----------------------------------------------------------------------------

int TierWorkerSolveVIInternal(const TierSolverApi *api, Tier tier, bool force,
                              bool compare, bool *solved) {
    if (solved != NULL) *solved = false;
    int ret = kRuntimeError;
    if (!force && DbManagerTierStatus(tier) == kDbTierStatusSolved) {
        ret = kNoError;  // Success.
        goto _bailout;
    }

    /* Value Iteration main algorithm. */
    if (!Step0Initialize(api, tier)) goto _bailout;
    if (!Step1LoadChildren()) goto _bailout;
    if (!Step2SetupSolvingTier()) goto _bailout;
    Step3ScanTier();
    if (!Step4Iterate()) goto _bailout;
    Step5MarkDrawPositions();
    Step6FlushDb();
    if (compare && !CompareDb()) goto _bailout;
    if (solved != NULL) *solved = true;
    ret = kNoError;  // Success.

_bailout:
    Step7Cleanup();
    return ret;
}

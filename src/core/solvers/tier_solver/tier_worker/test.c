/**
 * @file test.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Tier worker testing module implementation.
 * @version 1.0.2
 * @date 2024-09-08
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

#include "core/solvers/tier_solver/tier_worker/test.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stdio.h>    // printf, fprintf, stderr

#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"
#include "libs/mt19937/mt19937-64.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#define PRAGMA(X) _Pragma(#X)
#define PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(k) PRAGMA(omp parallel for schedule(dynamic, k))
#define PRAGMA_OMP_CRITICAL(name) PRAGMA(omp critical(name))

// Otherwise, the following macros do nothing.
#else
#define PRAGMA
#define PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(k)
#define PRAGMA_OMP_CRITICAL(name)
#endif  // _OPENMP

static const TierSolverApi *api_internal;

static bool IsLegalPosition(Tier tier, Position pos) {
    TierPosition tier_position = {.tier = tier, .position = pos};

    return api_internal->IsLegalPosition(tier_position);
}

static bool IsPrimitive(Tier tier, Position pos) {
    TierPosition tier_position = {.tier = tier, .position = pos};
    
    return api_internal->Primitive(tier_position) != kUndecided;
}

static int TestTierSymmetryRemoval(Tier tier, Position position,
                                   Tier canonical_tier) {
    Position (*ApplySymm)(TierPosition, Tier) =
        api_internal->GetPositionInSymmetricTier;

    TierPosition self = {.tier = tier, .position = position};
    TierPosition symm = {.tier = canonical_tier};
    symm.position = ApplySymm(self, canonical_tier);

    // Test if getting symmetric position from the same tier returns the
    // position itself.
    Position self_in_self_tier = ApplySymm(self, self.tier);
    Position symm_in_symm_tier = ApplySymm(symm, symm.tier);
    if (self_in_self_tier != self.position) {
        return kTierSolverTestTierSymmetrySelfMappingError;
    } else if (symm_in_symm_tier != symm.position) {
        return kTierSolverTestTierSymmetrySelfMappingError;
    }

    // Skip the next test if both tiers are the same.
    if (tier == canonical_tier) return kTierSolverTestNoError;

    // Test if applying the symmetry twice returns the same position.
    Position self_in_symm_tier = ApplySymm(self, symm.tier);
    Position symm_in_self_tier = ApplySymm(symm, self.tier);
    TierPosition self_in_symm_tier_tier_position = {
        .tier = symm.tier, .position = self_in_symm_tier};
    TierPosition symm_in_self_tier_tier_position = {
        .tier = self.tier, .position = symm_in_self_tier};
    Position new_self = ApplySymm(self_in_symm_tier_tier_position, self.tier);
    Position new_symm = ApplySymm(symm_in_self_tier_tier_position, symm.tier);
    if (new_self != self.position) {
        return kTierSolverTestTierSymmetryInconsistentError;
    } else if (new_symm != symm.position) {
        return kTierSolverTestTierSymmetryInconsistentError;
    }

    return kTierSolverTestNoError;
}

static bool TestChildTiers(TierPosition parent,
                           const TierPositionArray *children,
                           const TierHashSet *canonical_child_tiers) {
    for (int64_t i = 0; i < children->size; ++i) {
        if (!TierHashSetContains(canonical_child_tiers,
                                 children->array[i].tier)) {
            char parent_name[kDbFileNameLengthMax + 1];
            char child_name[kDbFileNameLengthMax + 1];
            api_internal->GetTierName(parent.tier, parent_name);
            api_internal->GetTierName(children->array[i].tier, child_name);
            printf("Position %" PRIPos " in tier [%s] (#%" PRITier
                   ") generated a child position %" PRIPos
                   " in tier [%s] (#%" PRITier
                   "), which is not in the list of child tiers of the parent "
                   "tier.\n",
                   parent.position, parent_name, parent.tier,
                   children->array[i].position, child_name,
                   children->array[i].tier);
            return false;
        }
    }

    return true;
}

static int TestChildPositions(Tier tier, Position position,
                              const TierHashSet *canonical_child_tiers) {
    TierPosition parent = {.tier = tier, .position = position};
    TierPositionArray children =
        api_internal->GetCanonicalChildPositions(parent);
    int error = kTierSolverTestNoError;

    // Test if the child tier positions' tiers are actually from the canonical
    // child tiers generated by the TierSolverApi::GetChildTiers function.
    if (!TestChildTiers(parent, &children, canonical_child_tiers)) {
        TierPositionArrayDestroy(&children);
        return kTierSolverTestIllegalChildTierError;
    }

    for (int64_t i = 0; i < children.size; ++i) {
        TierPosition child = children.array[i];
        bool in_range =
            (child.position >= 0) &&
            (child.position < api_internal->GetTierSize(child.tier));
        bool is_legal = api_internal->IsLegalPosition(child);
        if (!in_range || !is_legal) {
            error = kTierSolverTestIllegalChildPosError;
            break;
        }
    }
    TierPositionArrayDestroy(&children);

    return error;
}

static int TestChildToParentMatching(Tier tier, Position position) {
    TierPosition parent = {.tier = tier, .position = position};
    TierPosition canonical_parent = parent;
    canonical_parent.position =
        api_internal->GetCanonicalPosition(canonical_parent);
    TierPositionArray children =
        api_internal->GetCanonicalChildPositions(parent);
    int error = kTierSolverTestNoError;
    for (int64_t i = 0; i < children.size; ++i) {
        // Check if all child positions have parent as one of their parents.
        TierPosition child = children.array[i];
        PositionArray parents =
            api_internal->GetCanonicalParentPositions(child, tier);
        if (!PositionArrayContains(&parents, canonical_parent.position)) {
            error = kTierSolverTestChildParentMismatchError;
        }

        PositionArrayDestroy(&parents);
        if (error != kTierSolverTestNoError) break;
    }
    TierPositionArrayDestroy(&children);

    return error;
}

static int TestParentToChildMatching(Tier tier, Position position,
                                     const TierArray *parent_tiers) {
    TierPosition child = {.tier = tier, .position = position};
    TierPosition canonical_child = child;
    canonical_child.position =
        api_internal->GetCanonicalPosition(canonical_child);

    int error = kTierSolverTestNoError;
    for (int64_t i = 0; i < parent_tiers->size; ++i) {
        Tier parent_tier = parent_tiers->array[i];
        PositionArray parents = api_internal->GetCanonicalParentPositions(
            canonical_child, parent_tier);
        for (int64_t j = 0; j < parents.size; ++j) {
            // Skip illegal and primitive parent positions as they are also
            // skipped in solving.
            TierPosition parent = {.tier = parent_tier,
                                   .position = parents.array[j]};
            if (!api_internal->IsLegalPosition(parent)) continue;
            if (api_internal->Primitive(parent) != kUndecided) continue;

            // Check if all parent positions have child as one of their
            // children.
            TierPositionArray children =
                api_internal->GetCanonicalChildPositions(parent);
            if (!TierPositionArrayContains(&children, canonical_child)) {
                error = kTierSolverTestParentChildMismatchError;
            }

            TierPositionArrayDestroy(&children);
            if (error != kTierSolverTestNoError) break;
        }

        PositionArrayDestroy(&parents);
        if (error != kTierSolverTestNoError) break;
    }

    return error;
}

static void TestPrintError(Tier tier, Position position) {
    char name[kDbFileNameLengthMax + 1];
    if (api_internal->GetTierName(tier, name) != kNoError) {
        fprintf(stderr, "TestPrintError: (WARNING) GetTierName failed\n");
        sprintf(name, "GetTierName ERROR");
    }

    printf("\nTierWorkerTest: error detected at position %" PRIPos
           " of tier [%s] (#%" PRITier ")\n",
           position, name, tier);
}

static TierHashSet GetCanonicalChildTiers(const TierSolverApi *api,
                                          Tier parent) {
    TierHashSet ret;
    TierHashSetInit(&ret, 0.5);
    TierArray children = api->GetChildTiers(parent);
    for (int64_t i = 0; i < children.size; ++i) {
        Tier canonical = api->GetCanonicalTier(children.array[i]);
        if (TierHashSetContains(&ret, canonical)) continue;
        TierHashSetAdd(&ret, canonical);
    }
    TierArrayDestroy(&children);

    // Include the parent tier as well if it may loop back to itself.
    if (api_internal->GetTierType != kTierTypeImmediateTransition) {
        assert(api_internal->GetCanonicalTier(parent) == parent);
        TierHashSetAdd(&ret, parent);  // Assuming parent is canonical.
    }

    return ret;
}

int TierWorkerTestInternal(const TierSolverApi *api, Tier tier,
                           const TierArray *parent_tiers, long seed) {
    static const int64_t kTestSizeMax = 4096;  // Max positions to test.

    api_internal = api;
    init_genrand64(seed);  // Seed random number generator.
    int64_t tier_size = api_internal->GetTierSize(tier);
    bool random_test = tier_size > kTestSizeMax;
    int64_t test_size = random_test ? kTestSizeMax : tier_size;
    Tier canonical_tier = api_internal->GetCanonicalTier(tier);
    TierHashSet canonical_child_tiers = GetCanonicalChildTiers(api, tier);

    int error = kNoError;
    PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(16)
    for (int64_t i = 0; i < test_size; ++i) {
        if (error != kNoError) continue;  // Fail fast.
        long long next_rand64;
        PRAGMA_OMP_CRITICAL(mt19937) { next_rand64 = genrand64_int63(); }
        Position position = random_test ? next_rand64 % tier_size : i;

        // Skip if the current position is not legal.
        if (!IsLegalPosition(tier, position)) continue;

        // Check tier symmetry removal implementation.
        int local_error =
            TestTierSymmetryRemoval(tier, position, canonical_tier);
        if (local_error != kTierSolverTestNoError) {
            TestPrintError(tier, position);
            error = local_error;
            continue;
        }

        // Skip the following tests if the current position is primitive.
        if (IsPrimitive(tier, position)) continue;

        // Check if all child positions are legal.
        local_error =
            TestChildPositions(tier, position, &canonical_child_tiers);
        if (local_error != kTierSolverTestNoError) {
            TestPrintError(tier, position);
            error = local_error;
            continue;
        }

        // Perform the following tests only if current game variant implements
        // its own GetCanonicalParentPositions.
        if (api_internal->GetCanonicalParentPositions != NULL) {
            // Check if all child positions of the current position has the
            // current position as one of their parents.
            local_error = TestChildToParentMatching(tier, position);
            if (local_error != kTierSolverTestNoError) {
                TestPrintError(tier, position);
                error = local_error;
                continue;
            }

            // Check if all parent positions of the current position has the
            // current position as one of their children.
            local_error =
                TestParentToChildMatching(tier, position, parent_tiers);
            if (local_error != kTierSolverTestNoError) {
                TestPrintError(tier, position);
                error = local_error;
                continue;
            }
        }
    }
    TierHashSetDestroy(&canonical_child_tiers);

    return error;
}

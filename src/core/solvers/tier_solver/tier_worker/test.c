/**
 * @file test.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Tier worker testing module implementation.
 * @version 1.1.0
 * @date 2024-09-13
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

#include "core/concurrency.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"
#include "libs/mt19937/mt19937-64.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
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

static bool TestChildTiers(
    TierPosition parent,
    const TierPosition children[kTierSolverNumChildPositionsMax],
    int num_children, const TierHashSet *canonical_child_tiers) {
    //
    for (int i = 0; i < num_children; ++i) {
        if (!TierHashSetContains(canonical_child_tiers, children[i].tier)) {
            char parent_name[kDbFileNameLengthMax + 1];
            char child_name[kDbFileNameLengthMax + 1];
            api_internal->GetTierName(parent.tier, parent_name);
            api_internal->GetTierName(children[i].tier, child_name);
            printf("Position %" PRIPos " in tier [%s] (#%" PRITier
                   ") generated a child position %" PRIPos
                   " in tier [%s] (#%" PRITier
                   "), which is not in the list of child tiers of the parent "
                   "tier.\n",
                   parent.position, parent_name, parent.tier,
                   children[i].position, child_name, children[i].tier);
            return false;
        }
    }

    return true;
}

static TierPosition GetCanonicalTierPosition(TierPosition tier_position) {
    TierPosition canonical;

    // Convert to the tier position inside the canonical tier.
    canonical.tier = api_internal->GetCanonicalTier(tier_position.tier);
    if (canonical.tier == tier_position.tier) {  // Original tier is canonical.
        canonical.position = tier_position.position;
    } else {  // Original tier is not canonical.
        canonical.position = api_internal->GetPositionInSymmetricTier(
            tier_position, canonical.tier);
    }

    // Find the canonical position inside the canonical tier.
    canonical.position = api_internal->GetCanonicalPosition(canonical);

    return canonical;
}

static TierPositionHashSet ReferenceGetCanonicalChildPositions(
    TierPosition tier_position) {
    //
    TierPositionHashSet ret;
    TierPositionHashSetInit(&ret, 0.5);
    Move moves[kTierSolverNumMovesMax];
    int num_moves = api_internal->GenerateMoves(tier_position, moves);
    for (int i = 0; i < num_moves; ++i) {
        TierPosition child = api_internal->DoMove(tier_position, moves[i]);
        child = GetCanonicalTierPosition(child);
        if (!TierPositionHashSetContains(&ret, child)) {
            TierPositionHashSetAdd(&ret, child);
        }
    }

    return ret;
}

static int TestCustomGetCanonicalChildrenFunctions(
    TierPosition parent,
    const TierPosition canonical_children[kTierSolverNumChildPositionsMax],
    int num_children) {
    //
    TierPositionHashSet ref = ReferenceGetCanonicalChildPositions(parent);
    int ret = kTierSolverTestNoError;

    // Test if the sizes match.
    if (num_children != ref.size) {
        ret = kTierSolverTestGetCanonicalChildPositionsMismatch;
        goto _bailout;
    }

    // Test if the contents match.
    for (int i = 0; i < num_children; ++i) {
        if (!TierPositionHashSetContains(&ref, canonical_children[i])) {
            ret = kTierSolverTestGetCanonicalChildPositionsMismatch;
            goto _bailout;
        }
    }

    // Test if the custom GetNumberOfCanonicalChildPositions is correct.
    int num_canonical =
        api_internal->GetNumberOfCanonicalChildPositions(parent);
    if (num_canonical != ref.size) {
        ret = kTierSolverTestGetNumberOfCanonicalChildPositionsMismatch;
        goto _bailout;
    }

_bailout:
    TierPositionHashSetDestroy(&ref);
    return ret;
}

static int TestChildPositions(Tier tier, Position position,
                              const TierHashSet *canonical_child_tiers) {
    // Skip this test if the position is primitive, in which case
    // GetCanonicalChildPositions is undefined.
    if (IsPrimitive(tier, position)) return kTierSolverTestNoError;

    TierPosition parent = {.tier = tier, .position = position};
    TierPosition children[kTierSolverNumChildPositionsMax];
    int num_children =
        api_internal->GetCanonicalChildPositions(parent, children);
    int error = kTierSolverTestNoError;

    // Test if the child tier positions' tiers are actually from the canonical
    // child tiers generated by the TierSolverApi::GetChildTiers function.
    if (!TestChildTiers(parent, children, num_children,
                        canonical_child_tiers)) {
        return kTierSolverTestIllegalChildTierError;
    }

    // Test if the game-specific GetCanonicalChildPositions and
    // GetNumberOfCanonicalChildPositions are correctly implemented.
    error =
        TestCustomGetCanonicalChildrenFunctions(parent, children, num_children);
    if (error) return error;

    for (int64_t i = 0; i < num_children; ++i) {
        TierPosition child = children[i];
        bool in_range =
            (child.position >= 0) &&
            (child.position < api_internal->GetTierSize(child.tier));
        bool is_legal = api_internal->IsLegalPosition(child);
        if (!in_range || !is_legal) {
            error = kTierSolverTestIllegalChildPosError;
            break;
        }
    }

    return error;
}

static bool UsesLoopyAlgorithm(Tier tier) {
    return api_internal->GetTierType(tier) != kTierTypeImmediateTransition;
}

static int TestChildToParentMatching(Tier tier, Position position) {
    // Skip this test if the position is primitive, in which case
    // GetCanonicalChildPositions is undefined.
    if (IsPrimitive(tier, position)) return kTierSolverTestNoError;

    // The GetCanonicalParentPositions function does not need to work on parent
    // tiers that does not use the backward induction loopy algorithm.
    if (!UsesLoopyAlgorithm(tier)) return kTierSolverTestNoError;

    TierPosition parent = {.tier = tier, .position = position};
    TierPosition canonical_parent = parent;
    canonical_parent.position =
        api_internal->GetCanonicalPosition(canonical_parent);
    TierPosition children[kTierSolverNumChildPositionsMax];
    int num_children =
        api_internal->GetCanonicalChildPositions(parent, children);
    int error = kTierSolverTestNoError;
    for (int i = 0; i < num_children; ++i) {
        // Check if all child positions have parent as one of their parents.
        TierPosition child = children[i];
        Position parents[kTierSolverNumParentPositionsMax];
        int num_parents =
            api_internal->GetCanonicalParentPositions(child, tier, parents);
        bool found = false;
        for (int j = 0; j < num_parents; ++j) {
            if (parents[j] == canonical_parent.position) {
                found = true;
                break;
            }
        }
        if (!found) {
            error = kTierSolverTestChildParentMismatchError;
            break;
        }
    }

    return error;
}

static void PrintTierPositionAsStringIfPossible(TierPosition tp) {
    char *buf;
    if (api_internal->TierPositionToString) {
        buf = (char *)SafeCalloc(api_internal->position_string_length_max + 1,
                                 sizeof(char));
        api_internal->TierPositionToString(tp, buf);
    } else {
        buf = (char *)SafeCalloc(1, sizeof(char));
    }
    printf("%s\n", buf);
    GamesmanFree(buf);
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

        // The GetCanonicalParentPositions function does not need to work on
        // parent tiers that do not use the backward induction loopy algorithm.
        if (!UsesLoopyAlgorithm(parent_tier)) continue;

        Position parents[kTierSolverNumParentPositionsMax];
        int num_parents = api_internal->GetCanonicalParentPositions(
            canonical_child, parent_tier, parents);
        for (int j = 0; j < num_parents; ++j) {
            // Skip illegal and primitive parent positions as they are also
            // skipped in solving.
            TierPosition parent = {.tier = parent_tier, .position = parents[j]};
            if (!api_internal->IsLegalPosition(parent)) continue;
            if (api_internal->Primitive(parent) != kUndecided) continue;

            // Check if all parent positions have child as one of their
            // children.
            TierPosition children[kTierSolverNumChildPositionsMax];
            int num_children =
                api_internal->GetCanonicalChildPositions(parent, children);
            bool found = false;
            for (int k = 0; k < num_children; ++k) {
                if (children[k].position == canonical_child.position) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                error = kTierSolverTestParentChildMismatchError;
                printf("Parnet position: %" PRIPos "\n", parent.position);
                PrintTierPositionAsStringIfPossible(parent);
                break;
            }
        }
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

    char *buf;
    if (api_internal->TierPositionToString) {
        buf = (char *)SafeCalloc(api_internal->position_string_length_max + 1,
                                 sizeof(char));
        TierPosition tp = {.tier = tier, .position = position};
        api_internal->TierPositionToString(tp, buf);
    } else {
        buf = (char *)SafeCalloc(1, sizeof(char));
    }

    printf("\nTierWorkerTest: error detected at position %" PRIPos
           " of tier [%s] (#%" PRITier ")\n%s\n",
           position, name, tier, buf);
    GamesmanFree(buf);
}

static TierHashSet GetCanonicalChildTiers(const TierSolverApi *api,
                                          Tier parent) {
    TierHashSet ret;
    TierHashSetInit(&ret, 0.5);
    Tier children[kTierSolverNumChildTiersMax];
    int num_children = api->GetChildTiers(parent, children);
    for (int i = 0; i < num_children; ++i) {
        Tier canonical = api->GetCanonicalTier(children[i]);
        if (TierHashSetContains(&ret, canonical)) continue;
        TierHashSetAdd(&ret, canonical);
    }

    // Include the parent tier as well if it may loop back to itself.
    if (api_internal->GetTierType(parent) != kTierTypeImmediateTransition) {
        assert(api_internal->GetCanonicalTier(parent) == parent);
        TierHashSetAdd(&ret, parent);  // Assuming parent is canonical.
    }

    return ret;
}

int TierWorkerTestInternal(const TierSolverApi *api, Tier tier,
                           const TierArray *parent_tiers, long seed,
                           int64_t test_size) {
    api_internal = api;
    init_genrand64(seed);  // Seed random number generator.
    int64_t tier_size = api_internal->GetTierSize(tier);
    bool random_test = tier_size > test_size;  // Cap test size at tier size.
    if (!random_test) test_size = tier_size;
    Tier canonical_tier = api_internal->GetCanonicalTier(tier);
    TierHashSet canonical_child_tiers = GetCanonicalChildTiers(api, tier);
    ConcurrentInt error;
    ConcurrentIntInit(&error, kTierSolverTestNoError);

    PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(16)
    for (int64_t i = 0; i < test_size; ++i) {
        if (ConcurrentIntLoad(&error) != kTierSolverTestNoError) {
            continue;  // Fail fast.
        }

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
            ConcurrentIntStore(&error, local_error);
            continue;
        }

        // Check if all child positions are legal.
        local_error =
            TestChildPositions(tier, position, &canonical_child_tiers);
        if (local_error != kTierSolverTestNoError) {
            TestPrintError(tier, position);
            ConcurrentIntStore(&error, local_error);
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
                ConcurrentIntStore(&error, local_error);
                continue;
            }

            // Check if all parent positions of the current position has the
            // current position as one of their children.
            local_error =
                TestParentToChildMatching(tier, position, parent_tiers);
            if (local_error != kTierSolverTestNoError) {
                TestPrintError(tier, position);
                ConcurrentIntStore(&error, local_error);
                continue;
            }
        }
    }
    TierHashSetDestroy(&canonical_child_tiers);

    return ConcurrentIntLoad(&error);
}

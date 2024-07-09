#include "core/solvers/tier_solver/tier_worker/test.h"

#include <stdbool.h>  // bool, true, false
#include <stdio.h>    // printf

#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"
#include "libs/mt19937/mt19937-64.h"

static const TierSolverApi *api_internal;

static bool TestShouldSkip(Tier tier, Position position) {
    TierPosition tier_position = {.tier = tier, .position = position};
    if (!api_internal->IsLegalPosition(tier_position)) return true;
    if (api_internal->Primitive(tier_position) != kUndecided) return true;

    return false;
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

static int TestChildPositions(Tier tier, Position position) {
    TierPosition parent = {.tier = tier, .position = position};
    TierPositionArray children =
        api_internal->GetCanonicalChildPositions(parent);
    int error = kTierSolverTestNoError;
    for (int64_t i = 0; i < children.size; ++i) {
        TierPosition child = children.array[i];
        if (child.position < 0) {
            error = kTierSolverTestIllegalChildError;
            break;
        } else if (child.position >= api_internal->GetTierSize(child.tier)) {
            error = kTierSolverTestIllegalChildError;
            break;
        } else if (!api_internal->IsLegalPosition(child)) {
            error = kTierSolverTestIllegalChildError;
            break;
        }
    }

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

static int TestPrintError(Tier tier, Position position) {
    return printf("\nTierWorkerTest: error detected at position %" PRIPos
                  " of tier %" PRITier "\n",
                  position, tier);
}

int TierWorkerTestInternal(const TierSolverApi *api, Tier tier,
                           const TierArray *parent_tiers, long seed) {
    static const int64_t kTestSizeMax = 1000;  // Max positions to test.

    api_internal = api;
    init_genrand64(seed);  // Seed random number generator.
    int64_t tier_size = api_internal->GetTierSize(tier);
    bool random_test = tier_size > kTestSizeMax;
    int64_t test_size = random_test ? kTestSizeMax : tier_size;
    Tier canonical_tier = api_internal->GetCanonicalTier(tier);

    for (int64_t i = 0; i < test_size; ++i) {
        Position position = random_test ? genrand64_int63() % tier_size : i;
        if (TestShouldSkip(tier, position)) continue;

        // Check tier symmetry removal implementation.
        int error = TestTierSymmetryRemoval(tier, position, canonical_tier);
        if (error != kTierSolverTestNoError) {
            TestPrintError(tier, position);
            return error;
        }

        // Check if all child positions are legal.
        error = TestChildPositions(tier, position);
        if (error != kTierSolverTestNoError) {
            TestPrintError(tier, position);
            return error;
        }

        // Perform the following tests only if current game variant implements
        // its own GetCanonicalParentPositions.
        if (api_internal->GetCanonicalParentPositions != NULL) {
            // Check if all child positions of the current position has the
            // current position as one of their parents.
            error = TestChildToParentMatching(tier, position);
            if (error != kTierSolverTestNoError) {
                TestPrintError(tier, position);
                return error;
            }

            // Check if all parent positions of the current position has the
            // current position as one of their children.
            error = TestParentToChildMatching(tier, position, parent_tiers);
            if (error != kTierSolverTestNoError) {
                TestPrintError(tier, position);
                return error;
            }
        }
    }

    return kTierSolverTestNoError;
}

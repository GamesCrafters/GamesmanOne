#include "games/mtttier/mtttier.h"

#include <stdbool.h>  // bool, true, false
#include <stdint.h>

#include "core/gamesman.h"  // tier_solver
#include "core/gamesman_types.h"
#include "core/misc.h"  // Gamesman types utilities

// API Functions

static int64_t MtttierGetTierSize(Tier tier);
static MoveArray MtttierGenerateMoves(Tier tier, Position position);
static Value MtttierPrimitive(Tier tier, Position position);
static TierPosition MtttierDoMove(Tier tier, Position position, Move move);
static bool MtttierIsLegalPosition(Tier tier, Position position);
static Position MtttierGetCanonicalPosition(Tier tier, Position position);
static PositionArray MtttierGetCanonicalParentPositions(Tier tier,
                                                        Position position,
                                                        Tier parent_tier);
static TierArray MtttierGetChildTiers(Tier tier);
static TierArray MtttierGetParentTiers(Tier tier);

// ----------------------------------------------------------------------------
void MtttierInit(void) {
    tier_solver.GetTierSize = &MtttierGetTierSize;
    tier_solver.GenerateMoves = &MtttierGenerateMoves;
    tier_solver.Primitive = &MtttierPrimitive;
    tier_solver.DoMove = &MtttierDoMove;
    tier_solver.IsLegalPosition = &MtttierIsLegalPosition;
    tier_solver.GetCanonicalPosition = &MtttierGetCanonicalPosition;
    tier_solver.GetCanonicalParentPositions =
        &MtttierGetCanonicalParentPositions;
}
// ----------------------------------------------------------------------------

static int64_t MtttierGetTierSize(Tier tier) {
    return 19683;  // Naive hash.
}

static MoveArray MtttierGenerateMoves(Tier tier, Position position) {
    
}

static Value MtttierPrimitive(Tier tier, Position position) {}

static TierPosition MtttierDoMove(Tier tier, Position position, Move move) {}

static bool MtttierIsLegalPosition(Tier tier, Position position) {}

static Position MtttierGetCanonicalPosition(Tier tier, Position position) {}

static PositionArray MtttierGetCanonicalParentPositions(Tier tier,
                                                        Position position,
                                                        Tier parent_tier) {
    PositionArray parents;
    PositionArrayInit(&parents);

    return parents;
}

static TierArray MtttierGetChildTiers(Tier tier) {
    TierArray children;
    TierArrayInit(&children);
    if (tier < 9) TierArrayAppend(&children, tier + 1);
    return children;
}

static TierArray MtttierGetParentTiers(Tier tier) {
    TierArray parents;
    TierArrayInit(&parents);
    if (tier > 0) TierArrayAppend(&parents, tier - 1);
    return parents;
}

#include "games/gates/gates_tier.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stdint.h>   // uint8_t
#include <stdio.h>    // sprintf

#include "core/misc.h"
#include "core/types/gamesman_types.h"

// =============================== GatesTierHash ===============================

Tier GatesTierHash(const GatesTier *t) {
    Tier ret = 0;
    ret |= (Tier)t->phase << 0;
    ret |= (Tier)t->n_G << 2;
    ret |= (Tier)t->n_g << 4;
    ret |= (Tier)t->n_A << 6;
    ret |= (Tier)t->n_a << 8;
    ret |= (Tier)t->n_Z << 10;
    ret |= (Tier)t->n_z << 12;
    ret |= (Tier)t->G1 << 14;
    ret |= (Tier)t->G2 << 19;

    return ret;
}

// ============================== GatesTierUnhash ==============================

void GatesTierUnhash(Tier hash, GatesTier *dest) {
    dest->phase = (GatesTierField)((hash >> 0) & 0x03);  // 2 bits.
    dest->n_G = (GatesTierField)((hash >> 2) & 0x03);    // 2 bits.
    dest->n_g = (GatesTierField)((hash >> 4) & 0x03);    // 2 bits.
    dest->n_A = (GatesTierField)((hash >> 6) & 0x03);    // 2 bits.
    dest->n_a = (GatesTierField)((hash >> 8) & 0x03);    // 2 bits.
    dest->n_Z = (GatesTierField)((hash >> 10) & 0x03);   // 2 bits.
    dest->n_z = (GatesTierField)((hash >> 12) & 0x03);   // 2 bits.
    dest->G1 = (GatesTierField)((hash >> 14) & 0x1F);    // 5 bits.
    dest->G2 = (GatesTierField)((hash >> 19) & 0x1F);    // 5 bits.
}

// ============================ GatesGetInitialTier ============================

Tier GatesGetInitialTier(void) {
    GatesTier t = {0};
    assert(kPlacement == 0);

    return GatesTierHash(&t);
}

// ============================ GatesGetChildTiers ============================

static void GetChildTiersPlacementFewerThan11ByAddingNonWhiteGate(
    const GatesTier *t, TierArray *ret) {
    // Child GatesTier.
    GatesTier ct = *t;

    // Append all child tiers by adding a non-white-gate piece.
    if (t->n_A < 2) {
        ++ct.n_A;
        TierArrayAppend(ret, GatesTierHash(&ct));
        --ct.n_A;
    }
    if (t->n_a < 2) {
        ++ct.n_a;
        TierArrayAppend(ret, GatesTierHash(&ct));
        --ct.n_a;
    }
    if (t->n_g < 2) {
        ++ct.n_g;
        TierArrayAppend(ret, GatesTierHash(&ct));
        --ct.n_g;
    }
    if (t->n_Z < 2) {
        ++ct.n_Z;
        TierArrayAppend(ret, GatesTierHash(&ct));
        --ct.n_Z;
    }
    if (t->n_z < 2) {
        ++ct.n_z;
        TierArrayAppend(ret, GatesTierHash(&ct));
        --ct.n_z;
    }
}

static void SwapG(GatesTier *t) {
    GatesTierField tmp = t->G1;
    t->G1 = t->G2;
    t->G2 = tmp;
}

static void GetChildTiersPlacementFewerThan11ByAddingWhiteGate(
    const GatesTier *t, TierArray *ret) {
    // Child GatesTier.
    GatesTier ct = *t;

    switch (t->n_G) {
        case 0:  // No white gates have been placed.
            ++ct.n_G;
            for (ct.G1 = 0; ct.G1 < 18; ++ct.G1) {
                TierArrayAppend(ret, GatesTierHash(&ct));
            }
            break;

        case 1:  // One white gate has been placed.
            ++ct.n_G;
            for (ct.G2 = 0; ct.G2 < 18; ++ct.G2) {
                if (ct.G1 == ct.G2) continue;  // Cannot overlap with the first.
                bool swap = ct.G1 > ct.G2;
                if (swap) SwapG(&ct);  // Enforce G1 < G2.
                TierArrayAppend(ret, GatesTierHash(&ct));
                if (swap) SwapG(&ct);  // Revert swap.
            }
            break;

        case 2:     // Both white gates have been placed.
            break;  // Cannot add another.

        default:
            NotReached("GetChildTiersPlacement: invalid number of white gates");
            break;
    }
}

static TierArray GetChildTiersPlacementFewerThan11(const GatesTier *t) {
    assert(t->n_A + t->n_a + t->n_G + t->n_g + t->n_Z + t->n_z < 11);
    TierArray ret;
    TierArrayInit(&ret);

    // Append all child tiers by adding a non-white-gate piece.
    GetChildTiersPlacementFewerThan11ByAddingNonWhiteGate(t, &ret);

    // Append all child tiers by adding a white gate.
    GetChildTiersPlacementFewerThan11ByAddingWhiteGate(t, &ret);

    return ret;
}

static void GetChildTiersPlacement11AddImmediateScoring(GatesTier *ct,
                                                        TierArray *ret) {
    assert(ct->n_G == 2 && ct->n_g == 2);
    assert(ct->n_A == 2 && ct->n_a == 2);
    assert(ct->n_Z == 2 && ct->n_z == 2);
    ct->phase = kGateMoving;

    // Since it's black's turn, only black spikes may be scored.
    ct->n_a = 1;  // Case 1: scoring black triangle.
    TierArrayAppend(ret, GatesTierHash(ct));
    ct->n_a = 2;  // Revert changes.

    ct->n_z = 1;  // Case 2: scoring black trapezoid.
    TierArrayAppend(ret, GatesTierHash(ct));
    ct->n_z = 2;  // Revert changes.
}

static TierArray GetChildTiersPlacement11ByAddingNonWhiteGate(
    const GatesTier *t) {
    // Child GatesTier.
    GatesTier ct = *t;
    assert(t->n_G == 2);
    TierArray ret;
    TierArrayInit(&ret);

    // Add the last piece.
    ct.n_g = ct.n_A = ct.n_a = ct.n_Z = ct.n_z = 2;

    // Case A: do not score and enter movement phase.
    ct.phase = kMovement;
    TierArrayAppend(&ret, GatesTierHash(&ct));

    // Case B: score immediately after placing the last piece and enter
    // gate-moving phase.
    GetChildTiersPlacement11AddImmediateScoring(&ct, &ret);

    return ret;
}

static TierArray GetChildTiersPlacement11ByAddingWhiteGate(const GatesTier *t) {
    // Child GatesTier.
    GatesTier ct = *t;
    assert(t->n_G == 1);
    TierArray ret;
    TierArrayInit(&ret);

    // Add the second white gate.
    ct.n_G = 2;

    // Case A: do not score and enter movement phase.
    ct.phase = kMovement;
    for (ct.G2 = 0; ct.G2 < 18; ++ct.G2) {
        if (ct.G1 == ct.G2) continue;  // Cannot overlap.
        bool swap = ct.G1 > ct.G2;
        if (swap) SwapG(&ct);  // Enforce G1 < G2.
        TierArrayAppend(&ret, GatesTierHash(&ct));
        if (swap) SwapG(&ct);  // Revert swap.
    }

    // Case B: score immediately after placing the second white gate and
    // enter gate-moving phase.
    for (ct.G2 = 0; ct.G2 < 18; ++ct.G2) {
        if (ct.G1 == ct.G2) continue;  // Cannot overlap.
        bool swap = ct.G1 > ct.G2;
        if (swap) SwapG(&ct);  // Enforce G1 < G2.
        GetChildTiersPlacement11AddImmediateScoring(&ct, &ret);
        if (swap) SwapG(&ct);  // Revert swap.
    }

    return ret;
}

static TierArray GetChildTiersPlacement11(const GatesTier *t) {
    if (t->n_G == 2) {  // The last piece to add is not a white gate.
        return GetChildTiersPlacement11ByAddingNonWhiteGate(t);
    }

    // The last piece to add is a white gate.
    return GetChildTiersPlacement11ByAddingWhiteGate(t);
}

static TierArray GetChildTiersPlacement(const GatesTier *t) {
    assert(t->phase == kPlacement);
    int num_pieces = t->n_A + t->n_a + t->n_G + t->n_g + t->n_Z + t->n_z;
    if (num_pieces < 11) {
        return GetChildTiersPlacementFewerThan11(t);
    }
    assert(num_pieces == 11);

    return GetChildTiersPlacement11(t);
}

static TierArray GetChildTiersMovement(const GatesTier *t) {
    assert(t->phase == kMovement);
    TierArray ret;
    TierArrayInit(&ret);
    GatesTier ct = *t;  // Child GatesTier.

    // Scoring must occur if there is a tier transition from the movement phase.
    ct.phase = kGateMoving;
    if (t->n_A > 0) {
        --ct.n_A;  // White scores a triangle.
        TierArrayAppend(&ret, GatesTierHash(&ct));
        ++ct.n_A;  // Revert the change.
    }
    if (t->n_a > 0) {
        --ct.n_a;  // Black scores a triangle.
        TierArrayAppend(&ret, GatesTierHash(&ct));
        ++ct.n_a;  // Revert the change.
    }
    if (t->n_Z > 0) {
        --ct.n_Z;  // White scores a trapezoid.
        TierArrayAppend(&ret, GatesTierHash(&ct));
        ++ct.n_Z;  // Revert the change.
    }
    if (t->n_z > 0) {
        --ct.n_z;  // Black scores a trapezoid.
        TierArrayAppend(&ret, GatesTierHash(&ct));
        ++ct.n_z;  // Revert the change.
    }

    return ret;
}

static void GetChildTiersAfterGateMovement(GatesTier *ct, TierArray *ret,
                                           bool white_turn) {
    //
    assert(ct->n_A + ct->n_a > 0 && ct->n_Z + ct->n_z > 0);
    assert(ct->G1 < ct->G2);

    // Case A: no immediate scoring.
    ct->phase = kMovement;
    TierArrayAppend(ret, GatesTierHash(ct));

    ct->phase = kGateMoving;
    if (white_turn) {
        // Case B: white immediately scores.
        if (ct->n_A > 0) {
            --ct->n_A;  // Case B.1: white immediately scores a triangle.
            TierArrayAppend(ret, GatesTierHash(ct));
            ++ct->n_A;  // Revert changes by Case B.1.
        }
        if (ct->n_Z > 0) {
            --ct->n_Z;  // Case B.2: white immediately scores a trapezoid.
            TierArrayAppend(ret, GatesTierHash(ct));
            ++ct->n_Z;  // Revert changes by Case B.2.
        }
    } else {
        // Case C: black immediately scores.
        if (ct->n_a > 0) {
            --ct->n_a;  // Case C.1: black immediately scores a triangle.
            TierArrayAppend(ret, GatesTierHash(ct));
            ++ct->n_a;  // Revert changes by Case C.1.
        }

        if (ct->n_z > 0) {
            --ct->n_z;  // Case C.2: black immediately scores a trapezoid.
            TierArrayAppend(ret, GatesTierHash(ct));
            ++ct->n_z;  // Revert changes by Case C.2.
        }
    }
}

static TierArray GetChildTiersGateMovingWhiteTurn(const GatesTier *t) {
    assert(t->n_A == 2 && t->n_Z == 2);
    assert(t->n_a + t->n_z < 4 && t->n_a + t->n_z > 0);
    TierArray ret;
    TierArrayInit(&ret);

    // Child GatesTier. Since white cannot move its own gates,
    // G1 and G2 remain constant throughout.
    GatesTier ct = *t;
    GetChildTiersAfterGateMovement(&ct, &ret, true);

    return ret;
}

static void GetChildTiersGateMovingBlackTurnHelper(const GatesTier *pt,
                                                   GatesTier *ct,
                                                   TierArray *ret) {
    // All possible G1 movements.
    for (ct->G1 = 0; ct->G1 < 18; ++ct->G1) {
        if (ct->G1 == ct->G2) continue;  // Cannot overlap with the other gate.
        bool swap = ct->G1 > ct->G2;
        if (swap) SwapG(ct);
        GetChildTiersAfterGateMovement(ct, ret, false);
        if (swap) SwapG(ct);
    }
    *ct = *pt;  // Revert all changes.

    // All possible G2 movements.
    for (ct->G2 = 0; ct->G2 < 18; ++ct->G2) {
        if (ct->G1 == ct->G2) continue;  // Cannot overlap with the other gate.
        if (ct->G2 == pt->G2) continue;  // (G1, G2) is double counted.
        bool swap = ct->G1 > ct->G2;
        if (swap) SwapG(ct);
        GetChildTiersAfterGateMovement(ct, ret, false);
        if (swap) SwapG(ct);
    }
    *ct = *pt;  // Revert all changes.
}

static TierArray GetChildTiersGateMovingBlackTurn(const GatesTier *t) {
    assert(t->n_a == 2 && t->n_z == 2);
    assert(t->n_A + t->n_Z < 4 && t->n_A + t->n_Z > 0);
    TierArray ret;
    TierArrayInit(&ret);
    GatesTier ct = *t;  // Child GatesTier.
    GetChildTiersGateMovingBlackTurnHelper(t, &ct, &ret);

    return ret;
}

static TierArray GetChildTiersGateMovingEitherTurn(const GatesTier *t) {
    assert(t->n_A + t->n_Z < 4 && t->n_A + t->n_Z > 0);
    assert(t->n_a + t->n_z < 4 && t->n_a + t->n_z > 0);
    TierArray ret;
    TierArrayInit(&ret);
    GatesTier ct = *t;  // Child GatesTier.

    // Case A: white's turn, no white gate movement.
    GetChildTiersAfterGateMovement(&ct, &ret, true);

    // Case B: black's turn, possible white gate movement.
    GetChildTiersGateMovingBlackTurnHelper(t, &ct, &ret);

    return ret;
}

static TierArray GetChildTiersGateMoving(const GatesTier *t) {
    assert(t->phase == kGateMoving);
    assert(t->n_A + t->n_Z != 0 || t->n_a + t->n_z != 0);

    // Check if the game is over.
    if (t->n_A + t->n_Z == 0 || t->n_a + t->n_z == 0) {
        TierArray empty;
        TierArrayInit(&empty);
        return empty;
    }

    // Check if the tier contains only positions of one of the players' turns.
    if (t->n_A + t->n_Z == 4) {
        // White could not have scored, so it is white's turn.
        return GetChildTiersGateMovingWhiteTurn(t);
    } else if (t->n_a + t->n_z == 4) {
        // Black could not have scored, so it is black's turn.
        return GetChildTiersGateMovingBlackTurn(t);
    }

    // Either player's turn.
    return GetChildTiersGateMovingEitherTurn(t);
}

// TODO: address API change for immediate tier transition optimization.
int flag = 0;
static TierHashSet p, m, g;
TierArray GatesGetChildTiers(Tier tier) {
    if (!flag) {
        TierHashSetInit(&p, 0.5);
        TierHashSetInit(&m, 0.5);
        TierHashSetInit(&g, 0.5);
        flag = 1;
    }

    GatesTier t;
    GatesTierUnhash(tier, &t);
    switch (t.phase) {
        case kPlacement:
            TierHashSetAdd(&p, tier);
            return GetChildTiersPlacement(&t);

        case kMovement:
            TierHashSetAdd(&m, tier);
            return GetChildTiersMovement(&t);

        case kGateMoving:
            TierHashSetAdd(&g, tier);
            return GetChildTiersGateMoving(&t);
    }

    TierArray error_ret;
    TierArrayInit(&error_ret);
    NotReached("GatesGetChildTiers: unknown phase value");

    return error_ret;  // Never reached.
}

void gtprintdebug(void) {
    printf("placement: %" PRId64 "\n", p.size);
    printf("movement: %" PRId64 "\n", m.size);
    printf("gate moving: %" PRId64 "\n", g.size);
}

// ============================= GatesGetTierName =============================

#define PRIField PRIu8
int GatesGetTierName(Tier tier, char name[static kDbFileNameLengthMax]) {
    GatesTier t;
    GatesTierUnhash(tier, &t);
    int count = 0;
    switch (t.phase) {
        case kPlacement:
            count += sprintf(name + count, "p_");
            break;

        case kMovement:
            count += sprintf(name + count, "m_");
            break;

        case kGateMoving:
            count += sprintf(name + count, "g_");
            break;

        default:
            return kIllegalGameTierError;
    }

    count += sprintf(name + count,
                     "%" PRIField "%" PRIField "%" PRIField "%" PRIField
                     "%" PRIField "%" PRIField,
                     t.n_G, t.n_g, t.n_A, t.n_a, t.n_Z, t.n_z);
    if (t.n_G > 0) count += sprintf(name + count, "_%" PRIField, t.G1);
    if (t.n_G > 1) count += sprintf(name + count, "_%" PRIField, t.G2);

    return kNoError;
}
#undef PRIField

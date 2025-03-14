#include "games/gates/gates_tier.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stdint.h>   // int8_t
#include <stdio.h>    // sprintf

#include "core/misc.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

// ====================== GatesTierGetSymmetryMatrixEntry ======================

static const GatesTierField kSymmetryMatrix[kNumSymmetries][kBoardSize] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17},  // original
    {7, 3, 0, 11, 8, 4, 1, 15, 12, 5, 2, 16, 13, 9, 6, 17, 14, 10},  // cw60
    {15, 11, 7, 16, 12, 8, 3, 17, 13, 4, 0, 14, 9, 5, 1, 10, 6, 2},  // cw120
    {17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},  // 180
    {10, 14, 17, 6, 9, 13, 16, 2, 5, 12, 15, 1, 4, 8, 11, 0, 3, 7},  // cw240
    {2, 6, 10, 1, 5, 9, 14, 0, 4, 13, 17, 3, 8, 12, 16, 7, 11, 15},  // cw300

    {2, 1, 0, 6, 5, 4, 3, 10, 9, 8, 7, 14, 13, 12, 11, 17, 16, 15},  // reflect
    {10, 6, 2, 14, 9, 5, 1, 17, 13, 4, 0, 16, 12, 8, 3, 15, 11, 7},  // rcw60
    {17, 14, 10, 16, 13, 9, 6, 15, 12, 5, 2, 11, 8, 4, 1, 7, 3, 0},  // rcw120
    {15, 16, 17, 11, 12, 13, 14, 7, 8, 9, 10, 3, 4, 5, 6, 0, 1, 2},  // r180
    {7, 11, 15, 3, 8, 12, 16, 0, 4, 13, 17, 1, 5, 9, 14, 2, 6, 10},  // rcw240
    {0, 3, 7, 1, 4, 8, 11, 2, 5, 12, 15, 6, 9, 13, 16, 10, 14, 17},  // rcw300
};

GatesTierField GatesTierGetSymmetryMatrixEntry(int8_t symm, GatesTierField i) {
    return kSymmetryMatrix[symm][i];
}

// =============================== GatesTierHash ===============================

Tier GatesTierHash(const GatesTier *t) {
    Tier ret = 0;
    for (int p = 0; p < kNumPieceTypes; ++p) {
        ret |= (Tier)t->n[p] << (p * 2);
    }
    ret |= (Tier)t->phase << 12;
    ret |= (Tier)t->G1 << 14;
    ret |= (Tier)t->G2 << 19;

    return ret;
}

// ============================== GatesTierUnhash ==============================

void GatesTierUnhash(Tier hash, GatesTier *dest) {
    for (int p = 0; p < kNumPieceTypes; ++p) {
        // 2 bits each.
        dest->n[p] = (GatesTierField)((hash >> (p * 2)) & 0x03);
    }
    dest->phase = (GatesTierField)((hash >> 12) & 0x03);  // 2 bits.
    dest->G1 = (GatesTierField)((hash >> 14) & 0x1F);     // 5 bits.
    dest->G2 = (GatesTierField)((hash >> 19) & 0x1F);     // 5 bits.
}

// =================================== SwapG ===================================

void SwapG(GatesTier *t) {
    GatesTierField tmp = t->G1;
    t->G1 = t->G2;
    t->G2 = tmp;
}

// =========================== GatesTierGetNumPieces ===========================

GatesTierField GatesTierGetNumPieces(const GatesTier *t) {
    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
    return t->n[G] + t->n[g] + t->n[A] + t->n[a] + t->n[Z] + t->n[z];
}

// ============================ GatesGetInitialTier ============================

Tier GatesGetInitialTier(void) {
    GatesTier t = {0};
    assert(kPlacement == 0);

    return GatesTierHash(&t);
}

// =========================== GatesGetCanonicalTier ===========================

Tier GatesGetCanonicalTier(Tier tier) {
    Tier ret = tier;
    GatesTier original, symm;
    GatesTierUnhash(tier, &original);
    symm = original;
    if (original.n[G] == 0) {
        return tier;
    } else if (original.n[G] == 1) {
        assert(original.G2 == 0);
        for (int i = 1; i < kNumSymmetries; ++i) {
            // Apply symmetry.
            symm.G1 = kSymmetryMatrix[i][original.G1];
            Tier symm_hash = GatesTierHash(&symm);
            if (symm_hash < ret) ret = symm_hash;
        }
    } else {
        assert(original.n[G] == 2);
        for (int i = 1; i < kNumSymmetries; ++i) {
            // Apply symmetry.
            symm.G1 = kSymmetryMatrix[i][original.G1];
            symm.G2 = kSymmetryMatrix[i][original.G2];
            assert(symm.G1 != symm.G2);
            if (symm.G1 > symm.G2) {
                SwapG(&symm);  // Enforce G1 < G2.
                if (symm.phase == kGate1Moving) {
                    symm.phase = kGate2Moving;
                } else if (symm.phase == kGate2Moving) {
                    symm.phase = kGate1Moving;
                }
            }
            Tier symm_hash = GatesTierHash(&symm);
            if (symm_hash < ret) ret = symm_hash;
            symm = original;
        }
    }

    return ret;
}

// ============================ GatesGetChildTiers ============================

static void GetChildTiersPlacementFewerThan11ByAddingNonWhiteGate(
    const GatesTier *t, Tier children[static kTierSolverNumChildTiersMax],
    int *num_children) {
    // Child GatesTier.
    GatesTier ct = *t;

    // Append all child tiers by adding a non-white-gate piece.
    static_assert(G == 0, "G must be of index 0");
    for (int p = 1; p < kNumPieceTypes; ++p) {
        if (t->n[p] < 2) {
            ++ct.n[p];
            children[(*num_children)++] = GatesTierHash(&ct);
            --ct.n[p];
        }
    }
}

static void GetChildTiersPlacementFewerThan11ByAddingWhiteGate(
    const GatesTier *t, Tier children[static kTierSolverNumChildTiersMax],
    int *num_children) {
    // Child GatesTier.
    GatesTier ct = *t;

    switch (t->n[G]) {
        case 0:  // No white gates have been placed.
            ++ct.n[G];
            for (ct.G1 = 0; ct.G1 < kBoardSize; ++ct.G1) {
                children[(*num_children)++] = GatesTierHash(&ct);
            }
            break;

        case 1:  // One white gate has been placed.
            ++ct.n[G];
            for (ct.G2 = 0; ct.G2 < kBoardSize; ++ct.G2) {
                if (ct.G1 == ct.G2) continue;  // Cannot overlap with the first.
                bool swap = ct.G1 > ct.G2;
                if (swap) SwapG(&ct);  // Enforce G1 < G2.
                children[(*num_children)++] = GatesTierHash(&ct);
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

static int GetChildTiersPlacementFewerThan11(
    const GatesTier *t, Tier children[static kTierSolverNumChildTiersMax]) {
    assert(GatesTierGetNumPieces(t) < 11);
    int ret = 0;

    // Append all child tiers by adding a non-white-gate piece.
    GetChildTiersPlacementFewerThan11ByAddingNonWhiteGate(t, children, &ret);

    // Append all child tiers by adding a white gate.
    GetChildTiersPlacementFewerThan11ByAddingWhiteGate(t, children, &ret);

    return ret;
}

static void GetChildTiersPlacement11AddImmediateScoring(
    GatesTier *ct, Tier children[static kTierSolverNumChildTiersMax],
    int *num_children) {
    //
    assert(ct->n[G] == 2 && ct->n[g] == 2);
    assert(ct->n[A] == 2 && ct->n[a] == 2);
    assert(ct->n[Z] == 2 && ct->n[z] == 2);

    // Since it's black's turn, only black spikes may be scored.
    ct->n[a] = 1;              // Case A: scoring black triangle.
    ct->phase = kGate1Moving;  // Case A.1: into the first gate
    children[(*num_children)++] = GatesTierHash(ct);
    ct->phase = kGate2Moving;  // Case A.2: into the second gate
    children[(*num_children)++] = GatesTierHash(ct);
    ct->n[a] = 2;  // Revert changes.

    ct->n[z] = 1;              // Case B: scoring black trapezoid.
    ct->phase = kGate1Moving;  // Case B.1: into the first gate
    children[(*num_children)++] = GatesTierHash(ct);
    ct->phase = kGate2Moving;  // Case B.2: into the second gate
    children[(*num_children)++] = GatesTierHash(ct);
    ct->n[z] = 2;  // Revert changes.
}

static int GetChildTiersPlacement11ByAddingNonWhiteGate(
    const GatesTier *t, Tier children[static kTierSolverNumChildTiersMax]) {
    // Child GatesTier.
    GatesTier ct = *t;
    assert(t->n[G] == 2);
    int ret = 0;

    // Add the last piece.
    for (int p = 1; p < kNumPieceTypes; ++p) {
        ct.n[p] = 2;
    }

    // Case A: do not score and enter movement phase.
    ct.phase = kMovement;
    children[ret++] = GatesTierHash(&ct);

    // Case B: score immediately after placing the last piece and enter
    // gate-moving phase.
    GetChildTiersPlacement11AddImmediateScoring(&ct, children, &ret);

    return ret;
}

static int GetChildTiersPlacement11ByAddingWhiteGate(
    const GatesTier *t, Tier children[static kTierSolverNumChildTiersMax]) {
    // Child GatesTier.
    GatesTier ct = *t;
    assert(t->n[G] == 1);
    int ret = 0;

    // Add the second white gate.
    ct.n[G] = 2;
    for (ct.G2 = 0; ct.G2 < kBoardSize; ++ct.G2) {
        if (ct.G1 == ct.G2) continue;  // Cannot overlap.
        bool swap = ct.G1 > ct.G2;
        if (swap) SwapG(&ct);  // Enforce G1 < G2.

        // Case A: do not score and enter movement phase.
        ct.phase = kMovement;
        children[ret++] = GatesTierHash(&ct);

        // Case B: score immediately after placing the second white gate and
        // enter gate-moving phase.
        GetChildTiersPlacement11AddImmediateScoring(&ct, children, &ret);
        if (swap) SwapG(&ct);  // Revert swap.
    }

    return ret;
}

static int GetChildTiersPlacement11(
    const GatesTier *t, Tier children[static kTierSolverNumChildTiersMax]) {
    //
    if (t->n[G] == 2) {  // The last piece to add is not a white gate.
        return GetChildTiersPlacement11ByAddingNonWhiteGate(t, children);
    }

    // The last piece to add is a white gate.
    return GetChildTiersPlacement11ByAddingWhiteGate(t, children);
}

static int GetChildTiersPlacement(
    const GatesTier *t, Tier children[static kTierSolverNumChildTiersMax]) {
    //
    assert(t->phase == kPlacement);
    GatesTierField num_pieces = GatesTierGetNumPieces(t);
    if (num_pieces < 11) {
        return GetChildTiersPlacementFewerThan11(t, children);
    }
    assert(num_pieces == 11);

    return GetChildTiersPlacement11(t, children);
}

static int GetChildTiersMovement(
    const GatesTier *t, Tier children[static kTierSolverNumChildTiersMax]) {
    //
    assert(t->phase == kMovement);
    GatesTier ct = *t;  // Child GatesTier.
    int ret = 0;

    // Scoring must occur if there is a tier transition from the movement phase.
    static_assert(kGate1Moving + 1 == kGate2Moving, "");
    static_assert(g == 1 && A == 2, "");
    for (ct.phase = kGate1Moving; ct.phase <= kGate2Moving; ++ct.phase) {
        for (int p = A; p < kNumPieceTypes; ++p) {
            if (t->n[p] > 0) {
                --ct.n[p];  // Score the piece.
                children[ret++] = GatesTierHash(&ct);
                ++ct.n[p];  // Revert the change.
            }
        }
    }

    return ret;
}

static void GetChildTiersAfterGateMovement(
    GatesTier *ct, Tier children[static kTierSolverNumChildTiersMax],
    int *num_children, bool white_turn) {
    //
    assert(ct->n[A] + ct->n[Z] > 0 && ct->n[a] + ct->n[z] > 0);
    assert(ct->G1 < ct->G2);

    // Case A: no immediate scoring.
    ct->phase = kMovement;
    children[(*num_children)++] = GatesTierHash(ct);

    static_assert(kGate1Moving + 1 == kGate2Moving, "");
    for (ct->phase = kGate1Moving; ct->phase <= kGate2Moving; ++ct->phase) {
        if (white_turn) {
            // Case B: white immediately scores.
            if (ct->n[A] > 0) {
                --ct->n[A];  // Case B.1: white immediately scores a triangle.
                children[(*num_children)++] = GatesTierHash(ct);
                ++ct->n[A];  // Revert changes by Case B.1.
            }
            if (ct->n[Z] > 0) {
                --ct->n[Z];  // Case B.2: white immediately scores a trapezoid.
                children[(*num_children)++] = GatesTierHash(ct);
                ++ct->n[Z];  // Revert changes by Case B.2.
            }
        } else {
            // Case C: black immediately scores.
            if (ct->n[a] > 0) {
                --ct->n[a];  // Case C.1: black immediately scores a triangle.
                children[(*num_children)++] = GatesTierHash(ct);
                ++ct->n[a];  // Revert changes by Case C.1.
            }

            if (ct->n[z] > 0) {
                --ct->n[z];  // Case C.2: black immediately scores a trapezoid.
                children[(*num_children)++] = GatesTierHash(ct);
                ++ct->n[z];  // Revert changes by Case C.2.
            }
        }
    }
}

static int GetChildTiersGateMovingWhiteTurn(
    const GatesTier *t, Tier children[static kTierSolverNumChildTiersMax]) {
    //
    assert(t->n[A] == 2 && t->n[Z] == 2);
    assert(t->n[a] + t->n[z] < 4 && t->n[a] + t->n[z] > 0);
    int ret = 0;

    // Child GatesTier. Since white cannot move its own gates,
    // G1 and G2 remain constant throughout.
    GatesTier ct = *t;
    GetChildTiersAfterGateMovement(&ct, children, &ret, true);

    return ret;
}

static void GetChildTiersGateMovingBlackTurnHelper(
    const GatesTier *pt, GatesTier *ct,
    Tier children[static kTierSolverNumChildTiersMax], int *num_children) {
    //
    if (pt->phase == kGate1Moving) {
        // All possible G1 movements.
        for (ct->G1 = 0; ct->G1 < kBoardSize; ++ct->G1) {
            // Cannot overlap with the other gate.
            if (ct->G1 == ct->G2) continue;
            bool swap = ct->G1 > ct->G2;
            if (swap) SwapG(ct);
            GetChildTiersAfterGateMovement(ct, children, num_children, false);
            if (swap) SwapG(ct);
        }
    } else {
        // All possible G2 movements.
        for (ct->G2 = 0; ct->G2 < kBoardSize; ++ct->G2) {
            // Cannot overlap with the other gate.
            if (ct->G1 == ct->G2) continue;
            bool swap = ct->G1 > ct->G2;
            if (swap) SwapG(ct);
            GetChildTiersAfterGateMovement(ct, children, num_children, false);
            if (swap) SwapG(ct);
        }
    }
    *ct = *pt;  // Revert all changes.
}

static int GetChildTiersGateMovingBlackTurn(
    const GatesTier *t, Tier children[static kTierSolverNumChildTiersMax]) {
    //
    assert(t->n[a] == 2 && t->n[z] == 2);
    assert(t->n[A] + t->n[Z] < 4 && t->n[A] + t->n[Z] > 0);

    GatesTier ct = *t;  // Child GatesTier.
    int ret = 0;
    GetChildTiersGateMovingBlackTurnHelper(t, &ct, children, &ret);

    return ret;
}

static int DeduplicateTierArray(Tier *dest, const Tier *src, int size) {
    TierHashSet dedup;
    TierHashSetInit(&dedup, 0.5);
    int ret = 0;
    for (int i = 0; i < size; ++i) {
        if (TierHashSetContains(&dedup, src[i])) continue;
        TierHashSetAdd(&dedup, src[i]);
        dest[ret++] = src[i];
    }
    TierHashSetDestroy(&dedup);

    return ret;
}

static int GetChildTiersGateMovingEitherTurn(
    const GatesTier *t, Tier children[static kTierSolverNumChildTiersMax]) {
    assert(t->n[A] + t->n[Z] < 4 && t->n[A] + t->n[Z] > 0);
    assert(t->n[a] + t->n[z] < 4 && t->n[a] + t->n[z] > 0);
    Tier raw[kTierSolverNumChildTiersMax];
    int num_raw = 0;
    GatesTier ct = *t;  // Child GatesTier.

    // Case A: white's turn, no white gate movement.
    GetChildTiersAfterGateMovement(&ct, raw, &num_raw, true);

    // Case B: black's turn, possible white gate movement.
    GetChildTiersGateMovingBlackTurnHelper(t, &ct, raw, &num_raw);

    return DeduplicateTierArray(children, raw, num_raw);
}

static int GetChildTiersGateMoving(
    const GatesTier *t, Tier children[static kTierSolverNumChildTiersMax]) {
    //
    assert(t->phase == kGate1Moving || t->phase == kGate2Moving);
    assert(t->n[A] + t->n[Z] != 0 || t->n[a] + t->n[z] != 0);

    // Check if the game is over.
    if (t->n[A] + t->n[Z] == 0 || t->n[a] + t->n[z] == 0) return 0;

    // Check if the tier contains only positions of one of the players' turns.
    if (t->n[A] + t->n[Z] == 4) {
        // White could not have scored, so it is white's turn.
        return GetChildTiersGateMovingWhiteTurn(t, children);
    } else if (t->n[a] + t->n[z] == 4) {
        // Black could not have scored, so it is black's turn.
        return GetChildTiersGateMovingBlackTurn(t, children);
    }

    // Either player's turn.
    return GetChildTiersGateMovingEitherTurn(t, children);
}

TierType GatesGetTierType(Tier tier) {
    GatesTier t;
    GatesTierUnhash(tier, &t);
    switch (t.phase) {
        case kPlacement:
        case kGate1Moving:
        case kGate2Moving:
            return kTierTypeImmediateTransition;
    }

    return kTierTypeLoopy;
}

int GatesGetChildTiers(Tier tier,
                       Tier children[static kTierSolverNumChildTiersMax]) {
    //
    GatesTier t;
    GatesTierUnhash(tier, &t);
    switch (t.phase) {
        case kPlacement:
            return GetChildTiersPlacement(&t, children);

        case kMovement:
            return GetChildTiersMovement(&t, children);

        case kGate1Moving:
        case kGate2Moving:
            return GetChildTiersGateMoving(&t, children);
    }

    NotReached("GatesGetChildTiers: unknown phase value");
    return -1;  // Never reached.
}

// ============================= GatesGetTierName =============================

#define PRIField PRId8
int GatesGetTierName(Tier tier, char name[static kDbFileNameLengthMax + 1]) {
    GatesTier t;
    GatesTierUnhash(tier, &t);
    int count = 0;
    switch (t.phase) {
        case kPlacement:
            count += sprintf(name + count, "p_");
            count += sprintf(name + count, "%" PRIField "%" PRIField, t.n[G],
                             t.n[g]);
            break;

        case kMovement:
            count += sprintf(name + count, "m_");
            break;

        case kGate1Moving:
            count += sprintf(name + count, "g1_");
            break;

        case kGate2Moving:
            count += sprintf(name + count, "g2_");
            break;

        default:
            return kIllegalGameTierError;
    }

    count += sprintf(name + count,
                     "%" PRIField "%" PRIField "%" PRIField "%" PRIField,
                     t.n[A], t.n[a], t.n[Z], t.n[z]);
    if (t.n[G] > 0) count += sprintf(name + count, "_%" PRIField, t.G1);
    if (t.n[G] > 1) sprintf(name + count, "_%" PRIField, t.G2);

    return kNoError;
}
#undef PRIField

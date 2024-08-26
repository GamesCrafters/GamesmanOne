#include "games/gates/gates.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdio.h>   // sprintf

#include "core/generic_hash/generic_hash.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"
#include "games/gates/gates_tier.h"

enum {
    kBoardSize = 18,
};

// ============================== kGatesSolverApi ==============================

static Position GatesGetInitialPosition(void) {
    static ConstantReadOnlyString kGatesInitialBoard = "------------------";

    return GenericHashHashLabel(GatesGetInitialTier(), kGatesInitialBoard, 1);
}

static int64_t GatesGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static MoveArray GatesGenerateMoves(TierPosition tier_position) {
    // TODO
    (void)tier_position;

    MoveArray ret;
    MoveArrayInit(&ret);

    return ret;
}

static Value GatesPrimitive(TierPosition tier_position) {
    // TODO
    (void)tier_position;
    return kErrorValue;
}

static TierPosition GatesDoMove(TierPosition tier_position, Move move) {
    // TODO
    (void)tier_position;
    (void)move;

    TierPosition ret;
    ret.tier = -1;
    ret.position = -1;

    return ret;
}

static bool GatesIsLegalPosition(TierPosition tier_position) {
    (void)tier_position;  // There's no easy way to tell if a position is legal.
    return true;
}

static Position GatesGetCanonicalPosition(TierPosition tier_position) {
    // TODO
    return tier_position.position;
}

static TierPositionArray GatesGetCanonicalChildPositions(
    TierPosition tier_position) {
    // TODO
    (void)tier_position;

    TierPositionArray ret;
    TierPositionArrayInit(&ret);

    return ret;
}

static const TierSolverApi kGatesSolverApi = {
    .GetInitialTier = GatesGetInitialTier,
    .GetInitialPosition = GatesGetInitialPosition,
    .GetTierSize = GatesGetTierSize,

    .GenerateMoves = GatesGenerateMoves,
    .Primitive = GatesPrimitive,
    .DoMove = GatesDoMove,
    .IsLegalPosition = GatesIsLegalPosition,
    .GetCanonicalPosition = GatesGetCanonicalPosition,
    .GetCanonicalChildPositions = GatesGetCanonicalChildPositions,

    .GetChildTiers = GatesGetChildTiers,
    .GetTierName = GatesGetTierName,
};

// ============================= kGatesGameplayApi =============================

static const GameplayApi kGatesGameplayApi = {
    .common = NULL,  // TODO
    .tier = NULL,    // TODO
};

// ================================= GatesInit =================================

static void PlacementIndexToGatesTier(int i, GatesTier *dest) {
    dest->n_G = i % 3;
    i /= 3;
    dest->n_g = i % 3;
    i /= 3;
    dest->n_A = i % 3;
    i /= 3;
    dest->n_a = i % 3;
    i /= 3;
    dest->n_Z = i % 3;
    i /= 3;
    dest->n_z = i;
}

static void FillPieceInitArray(const GatesTier *t, int piece_init[static 19]) {
    piece_init[1] = piece_init[2] = t->n_g;
    piece_init[4] = piece_init[5] = t->n_A;
    piece_init[7] = piece_init[8] = t->n_a;
    piece_init[10] = piece_init[11] = t->n_Z;
    piece_init[13] = piece_init[14] = t->n_z;
    piece_init[16] = piece_init[17] =
        kBoardSize - t->n_G - t->n_g - t->n_A - t->n_a - t->n_Z - t->n_z;
}

static int InitGenericHashPlacement(void) {
    GatesTier t = {0};
    int piece_init[] = {
        'g', 0, 2, 'A', 0, 2, 'a', 0, 2, 'Z', 0, 2, 'z', 0, 2, '-', 6, 18, -1,
    };  // This is a placeholder definition and will be filled inside the loop.
    t.phase = kPlacement;

    // For each possible combination of pieces on board: 3 ** 6 == 729.
    for (int i = 0; i < 729; ++i) {
        PlacementIndexToGatesTier(i, &t);
        int num_pieces = t.n_G + t.n_g + t.n_A + t.n_a + t.n_Z + t.n_z;
        int turn = num_pieces % 2 == 0 ? 1 : 2;
        FillPieceInitArray(&t, piece_init);
        Tier hashed;
        bool success = false;
        switch (t.n_G) {
            case 0:  // No white gates
                t.G1 = t.G2 = 0;
                hashed = GatesTierHash(&t);
                success = GenericHashAddContext(turn, kBoardSize, piece_init,
                                                NULL, hashed);
                if (!success) return kGenericHashError;
                break;

            case 1:  // Only one white gate has been placed
                t.G2 = 0;
                for (t.G1 = 0; t.G1 < kBoardSize; ++t.G1) {
                    hashed = GatesTierHash(&t);
                    success = GenericHashAddContext(turn, kBoardSize - 1,
                                                    piece_init, NULL, hashed);
                    if (!success) return kGenericHashError;
                }
                break;

            case 2:  // Both white gates have been placed
                for (t.G1 = 0; t.G1 < kBoardSize; ++t.G1) {
                    for (t.G2 = t.G1 + 1; t.G2 < kBoardSize; ++t.G2) {
                        hashed = GatesTierHash(&t);
                        success = GenericHashAddContext(
                            turn, kBoardSize - 2, piece_init, NULL, hashed);
                        if (!success) return kGenericHashError;
                    }
                }
                break;
        }
    }

    return kNoError;
}

static void MovementIndexToGatesTier(int i, GatesTier *dest) {
    dest->n_A = i % 3;
    i /= 3;
    dest->n_a = i % 3;
    i /= 3;
    dest->n_Z = i % 3;
    i /= 3;
    dest->n_z = i;
}

static int InitGenericHashMovement(void) {
    GatesTier t = {0};
    int piece_init[] = {
        'g', 0, 2, 'A', 0, 2, 'a', 0, 2, 'Z', 0, 2, 'z', 0, 2, '-', 6, 18, -1,
    };  // This is a placeholder definition and will be filled inside the loop.
    t.n_G = t.n_g = 2;  // All four gates must be present.
    // For each possible combination of non-gate pieces on board: 3 ** 4 == 81.
    // Note that some combinations are invalid. However, including them in the
    // Generic Hash system should not be a bug.
    for (int i = 0; i < 81; ++i) {
        MovementIndexToGatesTier(i, &t);
        FillPieceInitArray(&t, piece_init);
        for (t.G1 = 0; t.G1 < kBoardSize; ++t.G1) {
            for (t.G2 = t.G1 + 1; t.G2 < kBoardSize; ++t.G2) {
                // Movement phase, not moving a gate this turn.
                t.phase = kMovement;
                Tier hashed = GatesTierHash(&t);
                bool success = GenericHashAddContext(0, kBoardSize - 2,
                                                     piece_init, NULL, hashed);
                if (!success) return kGenericHashError;

                // Movement phase, also moving a gate this turn.
                t.phase = kGateMoving;
                hashed = GatesTierHash(&t);
                int turn = 0;
                if (t.n_A + t.n_Z == 4) {
                    turn = 1;  // White haven't scored, so it's white's turn.
                } else if (t.n_a + t.n_z == 4) {
                    turn = 2;  // Black haven't scored, so it's black's turn.
                };  // t.n_A + t.n_Z + t.n_a + t.n_z == 8 is an invalid tier.
                success = GenericHashAddContext(turn, kBoardSize - 2,
                                                piece_init, NULL, hashed);
                if (!success) return kGenericHashError;
            }
        }
    }

    return kNoError;
}

static int InitGenericHash(void) {
    GenericHashReinitialize();
    int error = InitGenericHashPlacement();
    if (error) return error;
    error = InitGenericHashMovement();
    if (error) return error;

    return kNoError;
}

static int GatesInit(void *aux) {
    (void)aux;  // Unused.
    return InitGenericHash();
}

// =============================== GatesFinalize ===============================

static int GatesFinalize(void) { return kNoError; }

// ================================== kGates ==================================

const Game kGates = {
    .name = "gates",
    .formal_name = "Gates",
    .solver = &kTierSolver,
    .solver_api = &kGatesSolverApi,
    .gameplay_api = &kGatesGameplayApi,
    .uwapi = NULL,  // TODO

    .Init = GatesInit,
    .Finalize = GatesFinalize,

    .GetCurrentVariant = NULL,  // No other variants for now.
    .SetVariantOption = NULL,
};

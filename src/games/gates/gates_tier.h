#ifndef GAMESMANONE_GAMES_GATES_GATES_TIER_H_
#define GAMESMANONE_GAMES_GATES_GATES_TIER_H_

#include <inttypes.h>  // PRIu8
#include <stdint.h>    // int8_t

#include "core/types/gamesman_types.h"

enum {
    kBoardSize = 18,
    kNumSymmetries = 12,
};

typedef int8_t GatesTierField;

enum GatesPhase {
    kPlacement,
    kMovement,
    kGate1Moving,
    kGate2Moving,
};

enum GatesPieces { G, g, A, a, Z, z, kNumPieceTypes };

typedef struct {
    GatesTierField n[kNumPieceTypes];  // [0, 2] each
    GatesTierField phase;              // [0, 3]
    GatesTierField G1;                 // [0, 17]
    GatesTierField G2;                 // [G1 + 1, 17]
} GatesTier;

GatesTierField GatesTierGetSymmetryMatrixEntry(int8_t symm, GatesTierField i);

Tier GatesTierHash(const GatesTier *t);
void GatesTierUnhash(Tier hash, GatesTier *dest);

void SwapG(GatesTier *t);
GatesTierField GatesTierGetNumPieces(const GatesTier *t);

Tier GatesGetInitialTier(void);
Tier GatesGetCanonicalTier(Tier tier);
TierArray GatesGetChildTiers(Tier tier);
int GatesGetTierName(Tier tier, char name[static kDbFileNameLengthMax]);

#endif  // GAMESMANONE_GAMES_GATES_GATES_TIER_H_

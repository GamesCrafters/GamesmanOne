#ifndef GAMESMANONE_GAMES_GATES_GATES_TIER_H_
#define GAMESMANONE_GAMES_GATES_GATES_TIER_H_

#include <inttypes.h>  // PRIu8
#include <stdint.h>    // uint8_t

#include "core/types/gamesman_types.h"

typedef uint8_t GatesTierField;

enum GatesPhase {
    kPlacement,
    kMovement,
    kGateMoving,
};

typedef struct {
    GatesTierField phase;  // [0, 2]
    GatesTierField n_G;    // [0, 2]
    GatesTierField n_g;    // [0, 2]
    GatesTierField n_A;    // [0, 2]
    GatesTierField n_a;    // [0, 2]
    GatesTierField n_Z;    // [0, 2]
    GatesTierField n_z;    // [0, 2]
    GatesTierField G1;     // [0, 17]
    GatesTierField G2;     // [G1 + 1, 17]
} GatesTier;

Tier GatesTierHash(const GatesTier *t);
void GatesTierUnhash(Tier hash, GatesTier *dest);

Tier GatesGetInitialTier(void);
TierArray GatesGetChildTiers(Tier tier);
int GatesGetTierName(Tier tier, char name[static kDbFileNameLengthMax]);

#endif  // GAMESMANONE_GAMES_GATES_GATES_TIER_H_

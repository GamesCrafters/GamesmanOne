#include "gamesman.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t

#include "gamesman_types.h"
#include "misc.h"

Position global_num_positions = kDefaultGlobalNumberOfPositions;
Position global_initial_position = kDefaultInitialPosition;
Tier global_initial_tier = kDefaultInitialTier;

/* Regular, no tier */
MoveList (*GenerateMoves)(Position position) = NULL;
Value (*Primitive)(Position position) = NULL;
Position (*DoMove)(Position position, Move move) = NULL;
int (*GetNumberOfChildPositions)(Position position) = NULL;
PositionArray (*GetChildPositions)(Position position) = NULL;
PositionArray (*GetParentPositions)(Position position) = NULL;

/* Tier tree stuff */
TierArray (*GetChildTiers)(Tier tier) = NULL;
TierArray (*GetParentTiers)(Tier tier) = NULL;
bool (*IsCanonicalTier)(Tier tier) = NULL;
Tier (*GetCanonicalTier)(Tier tier) = NULL;

/* Tier position stuff */
int64_t (*GetTierSize)(Tier tier) = NULL;
MoveList (*TierGenerateMoves)(Tier tier, Position position) = NULL;
Value (*TierPrimitive)(Tier tier, Position position) = NULL;
TierPosition (*TierDoMove)(Tier tier, Position position, Move move) = NULL;
int (*TierGetNumberOfChildPositions)(Tier tier, Position position) = NULL;
PositionArray (*TierGetParentPositions)(Tier tier, Position position,
                                        Tier parent_tier) = NULL;

// Generic Derived API functions.
int GamesmanGetNumberOfChildPositions(Position position) {
    // Using a naive O(n^2) algorithm, assuming number is small.
    // May want to use a hash table instead if branching factor
    // can be big.
    PositionArray children;
    PositionArrayInit(&children);
    MoveList moves = GenerateMoves(position);
    for (MoveListItem *move = moves; move != NULL; move = move->next) {
        Position child = DoMove(position, move);
        if (!PositionArrayContains(&children, child)) {
            PositionArrayAppend(&children, child);
        }
    }
    MoveListDestroy(moves);
    // Again, we are assuming this number is small.
    int num_children = (int)children.size;
    PositionArrayDestroy(&children);
    return num_children;
}

// Duplicated child positions are allowed.
PositionArray GamesmanGetChildPositions(Position position) {
    PositionArray children;
    PositionArrayInit(&children);
    MoveList moves = GenerateMoves(position);
    for (MoveListItem *move = moves; move != NULL; move = move->next) {
        Position child = DoMove(position, move);
        PositionArrayAppend(&children, child);
    }
    MoveListDestroy(moves);
    return children;
}

// Only used by regular solver.
int64_t GamesmanGetTierSize(Tier tier) {
    (void)tier;
    return global_num_positions;
}

// Only used by regular solver.
MoveList GamesmanTierGenerateMoves(Tier tier, Position position) {
    (void)tier;
    return GenerateMoves(position);
}

// Only used by regular solver.
Value GamesmanTierPrimitive(Tier tier, Position position) {
    (void)tier;
    return Primitive(position);
}

// Only used by regular solver.
TierPosition GamesmanTierDoMove(Tier tier, Position position, Move move) {
    Position child = DoMove(position, move);
    TierPosition ret;
    ret.position = child;
    ret.tier = tier;
    return ret;
}

int GamesmanTierGetNumberOfChildPositions(Tier tier, Position position) {
    // TODO: need a TierPositionArray here...
}

PositionArray GamesmanTierGetParentPositions(Tier tier, Position position,
                                             Tier parent_tier) {}
#include "core/gamesman.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // exit

#include "core/gamesman_types.h"
#include "core/misc.h"

const Position kDefaultGlobalNumberOfPositions = -1;
const Position kTierGamesmanGlobalNumberOfPositions = 0;
const Position kDefaultInitialPosition = -1;
const Tier kDefaultInitialTier = -1;

Position global_num_positions = kDefaultGlobalNumberOfPositions;
Position global_initial_position = kDefaultInitialPosition;
Tier global_initial_tier = kDefaultInitialTier;
Analysis global_analysis = {0};

/* Regular, no tier */
MoveArray (*GenerateMoves)(Position position) = NULL;
Value (*Primitive)(Position position) = NULL;
Position (*DoMove)(Position position, Move move) = NULL;
bool (*IsLegalPosition)(Position position) = NULL;
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
MoveArray (*TierGenerateMoves)(Tier tier, Position position) = NULL;
Value (*TierPrimitive)(Tier tier, Position position) = NULL;
TierPosition (*TierDoMove)(Tier tier, Position position, Move move) = NULL;
bool (*TierIsLegalPosition)(Tier tier, Position position) = NULL;
int (*TierGetNumberOfChildPositions)(Tier tier, Position position) = NULL;
PositionArray (*TierGetParentPositions)(Tier tier, Position position,
                                        Tier parent_tier) = NULL;
Position (*TierGetNonCanonicalPosition)(Tier canonical_tier, Position position,
                                        Tier noncanonical_tier) = NULL;

// Generic Derived API functions.
int GamesmanGetNumberOfChildPositions(Position position) {
    // Using a naive O(n^2) algorithm, assuming number is small.
    // May want to use a hash table instead if branching factor
    // can be big.
    PositionArray children;
    PositionArrayInit(&children);
    MoveArray moves = GenerateMoves(position);
    for (int64_t i = 0; i < moves.size; ++i) {
        Position child = DoMove(position, moves.array[i]);
        if (!PositionArrayContains(&children, child)) {
            PositionArrayAppend(&children, child);
        }
    }
    MoveArrayDestroy(&moves);
    // Again, we are assuming this number is small.
    int num_children = (int)children.size;
    PositionArrayDestroy(&children);
    return num_children;
}

// Duplicated child positions are allowed.
PositionArray GamesmanGetChildPositions(Position position) {
    PositionArray children;
    PositionArrayInit(&children);
    MoveArray moves = GenerateMoves(position);
    for (int64_t i = 0; i < moves.size; ++i) {
        Position child = DoMove(position, moves.array[i]);
        PositionArrayAppend(&children, child);
    }
    MoveArrayDestroy(&moves);
    return children;
}

// Only used by regular solver.
int64_t GamesmanGetTierSizeConverted(Tier tier) {
    (void)tier;
    return global_num_positions;
}

// Only used by regular solver.
MoveArray GamesmanTierGenerateMovesConverted(Tier tier, Position position) {
    (void)tier;
    return GenerateMoves(position);
}

// Only used by regular solver.
Value GamesmanTierPrimitiveConverted(Tier tier, Position position) {
    (void)tier;
    return Primitive(position);
}

// Only used by regular solver.
TierPosition GamesmanTierDoMoveConverted(Tier tier, Position position,
                                         Move move) {
    Position child = DoMove(position, move);
    TierPosition ret;
    ret.position = child;
    ret.tier = tier;
    return ret;
}

bool GamesmanTierIsLegalPositionConverted(Tier tier, Position position) {
    (void)tier;
    return IsLegalPosition(position);
}

int GamesmanTierGetNumberOfChildPositionsConverted(Tier tier,
                                                   Position position) {
    (void)tier;
    return GetNumberOfChildPositions(position);
}

PositionArray GamesmanTierGetParentPositionsConverted(Tier tier,
                                                      Position position,
                                                      Tier parent_tier) {
    (void)tier;
    (void)parent_tier;
    return GetParentPositions(position);
}

TierArray GamesmanGetChildTiersConverted(Tier tier) {
    (void)tier;
    TierArray ret;
    Int64ArrayInit(&ret);
    return ret;
}

TierArray GamesmanGetParentTiersConverted(Tier tier) {
    (void)tier;
    TierArray ret;
    Int64ArrayInit(&ret);
    return ret;
}

bool GamesmanIsCanonicalTierConverted(Tier tier) {
    (void)tier;
    return true;
}

Tier GamesmanGetCanonicalTierConverted(Tier tier) { return tier; }

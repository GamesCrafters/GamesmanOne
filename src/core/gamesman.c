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

// Regular API.

MoveArray (*GenerateMoves)(Position position) = NULL;
Value (*Primitive)(Position position) = NULL;
Position (*DoMove)(Position position, Move move) = NULL;
bool (*IsLegalPosition)(Position position) = NULL;
int (*GetNumberOfCanonicalChildPositions)(Position position) = NULL;
PositionArray (*GetCanonicalChildPositions)(Position position) = NULL;
PositionArray (*GetCanonicalParentPositions)(Position position) = NULL;
Position (*GetCanonicalPosition)(Position position) = NULL;

// Tier position related API.

int64_t (*GetTierSize)(Tier tier) = NULL;
MoveArray (*TierGenerateMoves)(Tier tier, Position position) = NULL;
Value (*TierPrimitive)(Tier tier, Position position) = NULL;
TierPosition (*TierDoMove)(Tier tier, Position position, Move move) = NULL;
bool (*TierIsLegalPosition)(Tier tier, Position position) = NULL;
int (*TierGetNumberOfCanonicalChildPositions)(Tier tier,
                                              Position position) = NULL;
PositionArray (*TierGetCanonicalParentPositions)(Tier tier, Position position,
                                                 Tier parent_tier) = NULL;
Position (*TierGetCanonicalPosition)(Tier tier, Position position) = NULL;
Position (*TierGetPositionInNonCanonicalTier)(Tier canonical_tier,
                                              Position position,
                                              Tier noncanonical_tier) = NULL;

// Tier tree related API.

TierArray (*GetChildTiers)(Tier tier) = NULL;
TierArray (*GetParentTiers)(Tier tier) = NULL;
bool (*IsCanonicalTier)(Tier tier) = NULL;
Tier (*GetCanonicalTier)(Tier tier) = NULL;

Position GamesmanGetCanonicalPosition(Position position) { return position; }

int GamesmanGetNumberOfCanonicalChildPositions(Position position) {
    PositionHashSet children;
    PositionHashSetInit(&children, 0.5);
    MoveArray moves = GenerateMoves(position);
    for (int64_t i = 0; i < moves.size; ++i) {
        Position child = DoMove(position, moves.array[i]);
        child = GetCanonicalPosition(child);
        if (!PositionHashSetContains(&children, child)) {
            PositionHashSetAdd(&children, child);
        }
    }
    MoveArrayDestroy(&moves);
    int num_children = (int)children.size;
    PositionHashSetDestroy(&children);
    return num_children;
}

PositionArray GamesmanGetCanonicalChildPositions(Position position) {
    PositionArray children;
    PositionArrayInit(&children);
    MoveArray moves = GenerateMoves(position);
    for (int64_t i = 0; i < moves.size; ++i) {
        Position child = GetCanonicalPosition(DoMove(position, moves.array[i]));
        PositionArrayAppend(&children, child);
    }
    MoveArrayDestroy(&moves);
    return children;
}

int64_t GamesmanGetTierSizeConverted(Tier tier) {
    (void)tier;
    return global_num_positions;
}

MoveArray GamesmanTierGenerateMovesConverted(Tier tier, Position position) {
    (void)tier;
    return GenerateMoves(position);
}

Value GamesmanTierPrimitiveConverted(Tier tier, Position position) {
    (void)tier;
    return Primitive(position);
}

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

Position GamesmanTierGetCanonicalPositionConverted(Tier tier,
                                                   Position position) {
    (void)tier;
    return GetCanonicalPosition(position);
}

int GamesmanTierGetNumberOfCanonicalChildPositionsConverted(Tier tier,
                                                            Position position) {
    (void)tier;
    return GetNumberOfCanonicalChildPositions(position);
}

PositionArray GamesmanTierGetCanonicalParentPositionsConverted(
    Tier tier, Position position, Tier parent_tier) {
    (void)tier;
    (void)parent_tier;
    return GetCanonicalParentPositions(position);
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

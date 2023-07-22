#include "core/gamesman.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // exit

#include "core/gamesman_types.h"  // Tier, Position, MoveArray, etc.
#include "core/misc.h"            // Gamesman Types utilities

const Position kDefaultGlobalNumberOfPositions = -1;
const Position kTierGamesmanGlobalNumberOfPositions = 0;
const Position kDefaultInitialPosition = -1;
const Tier kDefaultInitialTier = -1;

Position global_num_positions = kDefaultGlobalNumberOfPositions;
Position global_initial_position = kDefaultInitialPosition;
Tier global_initial_tier = kDefaultInitialTier;
Analysis global_analysis = {0};

RegularSolverAPI regular_solver = {0};
TierSolverAPI tier_solver = {0};

Position GamesmanGetCanonicalPosition(Position position) { return position; }

int GamesmanGetNumberOfCanonicalChildPositions(Position position) {
    PositionHashSet children;
    PositionHashSetInit(&children, 0.5);
    MoveArray moves = regular_solver.GenerateMoves(position);
    for (int64_t i = 0; i < moves.size; ++i) {
        Position child = regular_solver.DoMove(position, moves.array[i]);
        child = regular_solver.GetCanonicalPosition(child);
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
    PositionHashSet deduplication_set;
    PositionHashSetInit(&deduplication_set, 0.5);

    PositionArray children;
    PositionArrayInit(&children);
    MoveArray moves = regular_solver.GenerateMoves(position);
    for (int64_t i = 0; i < moves.size; ++i) {
        Position child = regular_solver.DoMove(position, moves.array[i]);
        child = regular_solver.GetCanonicalPosition(child);
        if (!PositionHashSetContains(&deduplication_set, child)) {
            PositionHashSetAdd(&deduplication_set, child);
            PositionArrayAppend(&children, child);
        }
    }
    MoveArrayDestroy(&moves);
    PositionHashSetDestroy(&deduplication_set);
    return children;
}

int64_t GamesmanGetTierSizeConverted(Tier tier) {
    (void)tier;
    return global_num_positions;
}

MoveArray GamesmanTierGenerateMovesConverted(Tier tier, Position position) {
    (void)tier;
    return regular_solver.GenerateMoves(position);
}

Value GamesmanTierPrimitiveConverted(Tier tier, Position position) {
    (void)tier;
    return regular_solver.Primitive(position);
}

TierPosition GamesmanTierDoMoveConverted(Tier tier, Position position,
                                         Move move) {
    Position child = regular_solver.DoMove(position, move);
    TierPosition ret;
    ret.position = child;
    ret.tier = tier;
    return ret;
}

bool GamesmanTierIsLegalPositionConverted(Tier tier, Position position) {
    (void)tier;
    return regular_solver.IsLegalPosition(position);
}

Position GamesmanTierGetCanonicalPositionConverted(Tier tier,
                                                   Position position) {
    (void)tier;
    return regular_solver.GetCanonicalPosition(position);
}

int GamesmanTierGetNumberOfCanonicalChildPositionsConverted(Tier tier,
                                                            Position position) {
    (void)tier;
    return regular_solver.GetNumberOfCanonicalChildPositions(position);
}

TierPositionArray GamesmanTierGetCanonicalChildPositionsConverted(
    Tier tier, Position position) {
    (void)tier;
    PositionArray children =
        regular_solver.GetCanonicalChildPositions(position);
    TierPositionArray ret;
    TierPositionArrayInit(&ret);
    for (int64_t i = 0; i < children.size; ++i) {
        TierPosition this_child;
        this_child.position = children.array[i];
        this_child.tier = 0;  // Dummy value.
        TierPositionArrayAdd(&ret, this_child);
    }
    PositionArrayDestroy(&children);
    return ret;
}

PositionArray GamesmanTierGetCanonicalParentPositionsConverted(
    Tier tier, Position position, Tier parent_tier) {
    (void)tier;
    (void)parent_tier;
    return regular_solver.GetCanonicalParentPositions(position);
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

Tier GamesmanGetCanonicalTierConverted(Tier tier) { return tier; }

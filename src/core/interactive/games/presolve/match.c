#include "core/interactive/games/presolve/match.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // exit, EXIT_FAILURE
#include <string.h>   // memset

#include "core/constants.h"
#include "core/data_structures/int64_array.h"
#include "core/gamesman_types.h"
#include "core/solvers/solver_manager.h"

typedef struct Match {
    const Game *game;
    bool is_tier_game;
    TierPositionArray position_history;
    MoveArray move_history;
    Int64Array turn_history;  // Future-proof for go-again games.
    bool is_computer[2];
    bool solved;
} Match;

static Match match;

static void MatchDestroy(void);
static bool ImplementsBasicGameplayApi(const GameplayApi *api);
static bool ImplementsTierGameplayApi(const GameplayApi *api);
static bool ImplementsRegularGameplayApi(const GameplayApi *api);

// -----------------------------------------------------------------------------

const Game *InteractiveMatchGetCurrentGame(void) { return match.game; }

int InteractiveMatchSetGame(const Game *game) {
    MatchDestroy();
    memset(&match, 0, sizeof(match));
    match.game = game;
    game->Init(NULL);
    if (!ImplementsBasicGameplayApi(game->gameplay_api)) {
        return kInteractiveMatchSetGameBasicApiIncomplete;
    } else if (ImplementsRegularGameplayApi(game->gameplay_api)) {
        match.is_tier_game = false;
    } else if (ImplementsTierGameplayApi(game->gameplay_api)) {
        match.is_tier_game = true;
    } else {
        return kInteractiveMatchSetGameRegularOrTierApiIncomplete;
    }
    return kInteractiveMatchSetGameOk;
}

bool InteractiveMatchRestart(void) {
    if (match.game == NULL) return false;

    MatchDestroy();
    TierPositionArrayInit(&match.position_history);
    MoveArrayInit(&match.move_history);
    Int64ArrayInit(&match.turn_history);

    TierPosition initial_position;
    if (match.is_tier_game) {
        initial_position.tier = match.game->gameplay_api->GetInitialTier();
    } else {
        // By convention, all non-tier games use 0 as the only tier index.
        initial_position.tier = kDefaultTier;
    }
    initial_position.position = match.game->gameplay_api->GetInitialPosition();
    TierPositionArrayAppend(&match.position_history, initial_position);
    return true;
}

void InteractiveMatchTogglePlayerType(int player) {
    match.is_computer[player] = !match.is_computer[player];
}

bool InteractiveMatchPlayerIsComputer(int player) {
    return match.is_computer[player];
}

int InteractiveMatchGetCurrentVariant(void) {
    if (match.game->GetCurrentVariant == NULL) return 0;

    const GameVariant *variant = match.game->GetCurrentVariant();
    if (variant == NULL) return 0;

    return GameVariantToIndex(variant);
}

TierPosition InteractiveMatchGetCurrentPosition(void) {
    return TierPositionArrayBack(&match.position_history);
}

int InteractiveMatchGetTurn(void) {
    // This should be modified to support go-again games.
    return match.move_history.size % 2;
}

MoveArray InteractiveMatchGenerateMoves(void) {
    TierPosition current = TierPositionArrayBack(&match.position_history);
    if (match.is_tier_game) {
        return match.game->gameplay_api->TierGenerateMoves(current);
    }
    return match.game->gameplay_api->GenerateMoves(current.position);
}

TierPosition InteractiveMatchDoMove(TierPosition tier_position, Move move) {
    if (match.is_tier_game) {
        return match.game->gameplay_api->TierDoMove(tier_position, move);
    }
    return (TierPosition){
        .tier = kDefaultTier,
        .position =
            match.game->gameplay_api->DoMove(tier_position.position, move),
    };
}

bool InteractiveMatchCommitMove(Move move) {
    TierPosition current = TierPositionArrayBack(&match.position_history);
    TierPosition next = InteractiveMatchDoMove(current, move);

    if (!Int64ArrayPushBack(&match.turn_history, match.move_history.size % 2)) {
        return false;
    }
    if (!MoveArrayAppend(&match.move_history, move)) {
        Int64ArrayPopBack(&match.turn_history);
        return false;
    }
    if (!TierPositionArrayAppend(&match.position_history, next)) {
        Int64ArrayPopBack(&match.turn_history);
        MoveArrayPopBack(&match.move_history);
        return false;
    }
    return true;
}

Value InteractiveMatchPrimitive(void) {
    TierPosition current = TierPositionArrayBack(&match.position_history);
    if (match.is_tier_game) {
        return match.game->gameplay_api->TierPrimitive(current);
    }
    return match.game->gameplay_api->Primitive(current.position);
}

static int PreviousNonComputerMoveIndex(void) {
    int i = match.move_history.size - 1;
    while (i >= 0 && match.is_computer[match.turn_history.array[i]]) {
        --i;
    }
    return i;
}

bool InteractiveMatchUndo(void) {
    int new_move_history_size = PreviousNonComputerMoveIndex();
    if (new_move_history_size < 0) return false;

    // Equivalent to popping off all moves including and after the last human
    // move.
    match.move_history.size = match.turn_history.size = new_move_history_size;
    match.position_history.size = match.turn_history.size + 1;
    return true;
}

int InteractiveMatchPositionToString(TierPosition tier_position, char *buffer) {
    if (match.is_tier_game) {
        return match.game->gameplay_api->TierPositionToString(tier_position,
                                                              buffer);
    }
    return match.game->gameplay_api->PositionToString(tier_position.position,
                                                      buffer);
}

TierPosition InteractiveMatchGetCanonicalPosition(TierPosition tier_position) {
    TierPosition canonical = tier_position;

    // Convert to the tier position inside the canonical tier.
    if (match.game->gameplay_api->GetCanonicalTier != NULL &&
        match.game->gameplay_api->GetPositionInSymmetricTier != NULL) {
        //
        canonical.tier =
            match.game->gameplay_api->GetCanonicalTier(tier_position.tier);
        canonical.position =
            match.game->gameplay_api->GetPositionInSymmetricTier(
                tier_position, canonical.tier);
    }

    // Find the canonical position inside the canonical tier.
    if (match.is_tier_game &&
        match.game->gameplay_api->TierGetCanonicalPosition != NULL) {
        //
        canonical.position =
            match.game->gameplay_api->TierGetCanonicalPosition(canonical);
    } else if (!match.is_tier_game &&
               match.game->gameplay_api->GetCanonicalPosition != NULL) {
        canonical.position =
            match.game->gameplay_api->GetCanonicalPosition(canonical.position);
    }

    return canonical;
}

void InteractiveMatchSetSolved(bool solved) { match.solved = solved; }

bool InteractiveMatchSolved(void) { return match.solved; }

// -----------------------------------------------------------------------------

static void MatchDestroy(void) {
    TierPositionArrayDestroy(&match.position_history);
    MoveArrayDestroy(&match.move_history);
    Int64ArrayDestroy(&match.turn_history);
}

static bool ImplementsBasicGameplayApi(const GameplayApi *api) {
    if (api == NULL) return false;

    // Common.
    if (api->GetInitialPosition == NULL) return false;
    if (api->GetInitialPosition() < 0) return false;
    if (api->position_string_length_max <= 0) return false;

    // "Move to string" related functions.
    if (api->move_string_length_max <= 0) return false;
    if (api->MoveToString == NULL) return false;

    // "String to Move" related functions.
    if (api->IsValidMoveString == NULL) return false;
    if (api->StringToMove == NULL) return false;

    return true;
}

static bool ImplementsTierGameplayApi(const GameplayApi *api) {
    if (!ImplementsBasicGameplayApi(api)) return false;
    if (api->GetInitialTier == NULL) return false;
    if (api->GetInitialTier() < 0) return false;
    if (api->TierPositionToString == NULL) return false;
    if (api->TierGenerateMoves == NULL) return false;
    if (api->TierDoMove == NULL) return false;
    if (api->TierPrimitive == NULL) return false;

    return true;
}

static bool ImplementsRegularGameplayApi(const GameplayApi *api) {
    if (!ImplementsBasicGameplayApi(api)) return false;
    if (api->PositionToString == NULL) return false;
    if (api->GenerateMoves == NULL) return false;
    if (api->DoMove == NULL) return false;
    if (api->Primitive == NULL) return false;

    return true;
}

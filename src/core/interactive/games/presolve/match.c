#include "core/interactive/games/presolve/match.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // exit, EXIT_FAILURE

#include "core/data_structures/int64_array.h"
#include "core/gamesman_types.h"
#include "core/misc.h"

typedef struct Match {
    const Game *current_game;
    TierPositionArray position_history;
    MoveArray move_history;
    Int64Array turn_history;  // Future-proof for go-again games.
    bool is_computer[2];
} Match;

static Match match;

const Game *InteractiveMatchGetCurrentGame(void) { return match.current_game; }

static void MatchDestroy(void) {
    TierPositionArrayDestroy(&match.position_history);
    MoveArrayDestroy(&match.move_history);
    Int64ArrayDestroy(&match.turn_history);
}

void InteractiveMatchRestart(const Game *game) {
    MatchDestroy();
    TierPositionArrayInit(&match.position_history);
    MoveArrayInit(&match.move_history);
    Int64ArrayInit(&match.turn_history);
    match.current_game = game;
    TierPositionArrayAppend(
        &match.position_history,
        (TierPosition){
            .tier = game->gameplay_api->default_initial_tier,
            .position = game->gameplay_api->default_initial_position});
}

void InteractiveMatchTogglePlayerType(int player) {
    match.is_computer[player] = !match.is_computer[player];
}

bool InteractiveMatchPlayerIsComputer(int player) {
    return match.is_computer[player];
}

TierPosition InteractiveMatchGetCurrentPosition(void) {
    return TierPositionArrayBack(&match.position_history);
}

int InteractiveMatchGetTurn(void) {
    // This should be modified to support go-again games.
    return match.move_history.size % 2;
}

bool InteractiveMatchDoMove(Move move) {
    TierPosition current = TierPositionArrayBack(&match.position_history);
    TierPosition next;
    if (!Int64ArrayPushBack(&match.turn_history, match.move_history.size % 2)) {
        return false;
    }
    if (!MoveArrayAppend(&match.move_history, move)) {
        Int64ArrayPopBack(&match.turn_history);
        return false;
    }
    if (match.current_game->gameplay_api->TierDoMove != NULL) {
        next = match.current_game->gameplay_api->TierDoMove(
            current.tier, current.position, move);
    } else if (match.current_game->gameplay_api->DoMove != NULL) {
        next.tier = 0;
        next.position =
            match.current_game->gameplay_api->DoMove(current.position, move);
    } else {
        fprintf(stderr,
                "InteractiveMatchDoMove: (BUG) this function should not be "
                "called if current game does not have a working GameplayApi "
                "implemented. Aborting...\n");
        exit(EXIT_FAILURE);
    }
    if (!TierPositionArrayAppend(&match.position_history, next)) {
        Int64ArrayPopBack(&match.turn_history);
        MoveArrayPopBack(&match.move_history);
        return false;
    }
    return true;
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

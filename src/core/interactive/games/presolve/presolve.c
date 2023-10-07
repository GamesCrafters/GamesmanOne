#include "core/interactive/games/presolve/presolve.h"

#include <assert.h>   // assert
#include <stdbool.h>  // true
#include <stddef.h>   // NULL
#include <stdio.h>    // printf, fprintf, stderr
#include <stdlib.h>   // atoi

#include "core/gamesman_types.h"
#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/match.h"
#include "core/interactive/games/presolve/options/options.h"
#include "core/interactive/games/presolve/postsolve/postsolve.h"
#include "core/interactive/games/presolve/solver_options/solver_options.h"
#include "core/solvers/solver_manager.h"
#include "games/game_manager.h"

// Hard-coded size based on the title definition in UpdateTitle.
char title[43 + kGameFormalNameLengthMax + kUint32Base10StringLengthMax];

static int SetCurrentGame(ReadOnlyString key);
static void SolveAndStart(ReadOnlyString key);
static void UpdateTitle(void);

void InteractivePresolve(ReadOnlyString key) {
    int error = SetCurrentGame(key);
    if (error != 0) {
        fprintf(stderr,
                "InteractivePresolve: failed to set game. Error code %d\n",
                error);
        return;
    }

    static ConstantReadOnlyString items[] = {
        "Solve and start",
        "Start without solving",
        "Game options",
        "Solver options",
    };
    static ConstantReadOnlyString keys[] = {"s", "w", "g", "o"};
    static const HookFunctionPointer hooks[] = {
        &SolveAndStart,
        &InteractivePostSolve,
        &InteractiveGameOptions,
        &InteractiveSolverOptions,
    };
    int num_items = sizeof(items) / sizeof(items[0]);
    AutoMenu(title, num_items, items, keys, hooks, &UpdateTitle);
}

static int SetCurrentGame(ReadOnlyString key) {
    int game_index = atoi(key);
    assert(game_index >= 0 && game_index < GameManagerNumGames());

    const Game *const *all_games = GameManagerGetAllGames();
    const Game *current_game = all_games[game_index];
    int error = InteractiveMatchSetGame(current_game);
    if (error != 0) return error;

    return SolverManagerInitSolver(current_game);
}

static void SolveAndStart(ReadOnlyString key) {
    SolverManagerSolve(NULL);  // Auxiliary variable currently unused.
    InteractiveMatchSetSolved(true);
    InteractivePostSolve(key);
}

static void UpdateTitle(void) {
    const Game *current_game = InteractiveMatchGetCurrentGame();
    int variant_index = InteractiveMatchGetVariantIndex();
    sprintf(title, "Main (Pre-Solved) Menu for %s (variant %d)",
            current_game->formal_name, variant_index);
}

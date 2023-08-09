#include "core/interactive/games/presolve/presolve.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdio.h>   // printf
#include <stdlib.h>  // atoi

#include "core/gamesman_types.h"
#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/match.h"
#include "core/interactive/games/presolve/options/options.h"
#include "core/interactive/games/presolve/postsolve/postsolve.h"
#include "core/interactive/games/presolve/solver_options/solver_options.h"
#include "core/solver_manager.h"
#include "games/game_manager.h"

static void SetCurrentGame(const char *key) {
    int game_index = atoi(key);
    assert(game_index >= 0 && game_index < GameManagerNumGames());
    const Game *all_games = GameManagerGetAllGames();
    const Game *current_game = &all_games[game_index];
    InteractiveMatchRestart(current_game);
    SolverManagerInitSolver(current_game);
}

static void SolveAndStart(const char *key) {
    SolverManagerSolve(NULL);  // Auxiliary variable currently unused.
    InteractivePostSolve(key);
}

void InteractivePresolve(const char *key) {
    SetCurrentGame(key);
    const Game *current_game = InteractiveMatchGetCurrentGame();
    const GameVariant *variant = current_game->GetCurrentVariant();
    int variant_index = GameVariantToIndex(variant);

    // Hard-coded size based on the title definition below.
    char title[43 + kGameFormalNameLengthMax + kUint32Base10StringLengthMax];
    sprintf(title, "Main (Pre-Solved) Menu for %s (variant %d)",
            current_game->formal_name, variant_index);
    static const char *const items[] = {"Solve and start",
                                        "Start without solving", "Game options",
                                        "Solver options"};
    static const char *const keys[] = {"s", "w", "g", "o"};
    static const HookFunctionPointer hooks[] = {
        &SolveAndStart, &InteractivePostSolve, &InteractiveGameOptions,
        &InteractiveSolverOptions};
    AutoMenu(title, sizeof(items) / sizeof(items[0]), items, keys, hooks);
}

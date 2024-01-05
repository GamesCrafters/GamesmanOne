#include "core/interactive/games/presolve/presolve.h"

#include <assert.h>   // assert
#include <stdbool.h>  // true
#include <stddef.h>   // NULL
#include <stdio.h>    // printf, fprintf, stderr
#include <stdlib.h>   // atoi

#include "core/constants.h"
#include "core/types/gamesman_types.h"
#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/match.h"
#include "core/interactive/games/presolve/options/options.h"
#include "core/interactive/games/presolve/postsolve/postsolve.h"
#include "core/interactive/games/presolve/solver_options/solver_options.h"
#include "core/solvers/solver_manager.h"
#include "games/game_manager.h"

static int SetCurrentGame(ReadOnlyString key) {
    int game_index = atoi(key);
    assert(game_index >= 0 && game_index < GameManagerNumGames());

    // Aux parameter for game initialization currently unused.
    const Game *current_game = GameManagerInitGameIndex(game_index, NULL);
    assert(current_game != NULL);
    int error = InteractiveMatchSetGame(current_game);
    if (error != 0) return error;

    // TODO: add support to user-specified data_path.
    return SolverManagerInit(NULL);
}

static void SolveAndStart(ReadOnlyString key) {
    SolverManagerSolve(NULL);  // Auxiliary variable currently unused.
    InteractiveMatchSetSolved(true);
    InteractivePostSolve(key);
}

void InteractivePresolve(ReadOnlyString key) {
    int error = SetCurrentGame(key);
    if (error != 0) {
        fprintf(stderr,
                "InteractivePresolve: failed to set game. Error code %d\n",
                error);
        return;
    }

    const Game *current_game = InteractiveMatchGetCurrentGame();
    int variant_id = InteractiveMatchGetCurrentVariant();

    const char title_format[] = "Main (Pre-Solved) Menu for %s (variant %d)";
    char title[sizeof(title_format) + kGameFormalNameLengthMax +
               kUint32Base10StringLengthMax];
    sprintf(title, title_format, current_game->formal_name, variant_id);
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
    AutoMenu(title, sizeof(items) / sizeof(items[0]), items, keys, hooks);
}

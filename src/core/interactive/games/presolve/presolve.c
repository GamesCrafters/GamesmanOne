#include "core/interactive/games/presolve/presolve.h"

#include <assert.h>   // assert
#include <stdbool.h>  // true
#include <stddef.h>   // NULL
#include <stdio.h>    // printf, fprintf, stderr
#include <stdlib.h>   // atoi

#include "core/constants.h"
#include "core/game_manager.h"
#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/match.h"
#include "core/interactive/games/presolve/options/options.h"
#include "core/interactive/games/presolve/postsolve/postsolve.h"
#include "core/interactive/games/presolve/savio/partition_select.h"
#include "core/interactive/games/presolve/solver_options/solver_options.h"
#include "core/savio/scriptgen.h"
#include "core/solvers/solver_manager.h"
#include "core/types/gamesman_types.h"

static const char title_format[] = "Main (Pre-Solved) Menu for %s (variant %d)";
static char title[sizeof(title_format) + kGameFormalNameLengthMax +
                  kUint32Base10StringLengthMax];

static int SetCurrentGame(ReadOnlyString key);
#ifndef USE_MPI
static void SolveAndStart(ReadOnlyString key);
#endif
static void UpdateTitle(void);

// -----------------------------------------------------------------------------

void InteractivePresolve(ReadOnlyString key) {
    int error = SetCurrentGame(key);
    if (error != 0) {
        fprintf(stderr,
                "InteractivePresolve: failed to set game. Error code %d\n",
                error);
        return;
    }

    static ConstantReadOnlyString items[] = {
#ifndef USE_MPI
        "Solve and start",
#else
        "Generate SLURM job script for Savio",
#endif
        "Start without solving",
        "Game options",
        "Solver options",
    };
    static ConstantReadOnlyString keys[] = {"s", "w", "g", "o"};
    static const HookFunctionPointer hooks[] = {
#ifndef USE_MPI
        &SolveAndStart,
#else
        &InteractiveSavioPartitionSelect,
#endif
        &InteractivePostSolve,
        &InteractiveGameOptions,
        &InteractiveSolverOptions,
    };
    int num_items = sizeof(items) / sizeof(items[0]);
    AutoMenu(title, num_items, items, keys, hooks, &UpdateTitle);
}

// -----------------------------------------------------------------------------

static int SetCurrentGame(ReadOnlyString key) {
    int game_index = atoi(key);
    assert(game_index >= 0 && game_index < GameManagerNumGames());

    // Aux parameter for game initialization currently unused.
    const Game *current_game = GameManagerInitGameIndex(game_index, NULL);
    if (current_game == NULL) return kGameInitFailureError;

    int error = InteractiveMatchSetGame(current_game);
    if (error != 0) return error;

    // TODO: add support to user-specified data_path.
    return SolverManagerInit(NULL);
}

#ifndef USE_MPI
static void SolveAndStart(ReadOnlyString key) {
    SolverManagerSolve(NULL);  // Auxiliary variable currently unused.
    InteractiveMatchSetSolved(true);
    InteractivePostSolve(key);
}
#endif

static void UpdateTitle(void) {
    const Game *current_game = InteractiveMatchGetCurrentGame();
    int variant_index = InteractiveMatchGetVariantIndex();
    sprintf(title, title_format, current_game->formal_name, variant_index);
}

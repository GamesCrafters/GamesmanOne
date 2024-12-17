#include "core/interactive/games/presolve/presolve.h"

#include <assert.h>   // assert
#include <stdbool.h>  // true
#include <stddef.h>   // NULL
#include <stdio.h>    // printf, fprintf, stderr, scanf
#include <stdlib.h>   // atoi
#include <time.h>     // time

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

#ifndef USE_MPI
static const char title_format[] = "Main (Pre-Solved) Menu for %s (variant %d)";
#else   // USE_MPI is defined
static const char title_format[] =
    "Main (MPI Pre-Solved) Menu for %s (variant %d)";
#endif  // USE_MPI

static char title[sizeof(title_format) + kGameFormalNameLengthMax +
                  kUint32Base10StringLengthMax];

static int SetCurrentGame(ReadOnlyString key);
#ifndef USE_MPI
static int SolveAndStart(ReadOnlyString key);
#endif  // USE_MPI
static int TestCurrentGameVariant(ReadOnlyString key);
static void UpdateTitle(void);

// -----------------------------------------------------------------------------

int InteractivePresolve(ReadOnlyString key) {
    int error = SetCurrentGame(key);
    if (error != 0) {
        fprintf(stderr,
                "InteractivePresolve: failed to set game. Error code %d\n",
                error);
        return 0;
    }

    static ConstantReadOnlyString items[] = {
#ifndef USE_MPI
        "Solve and start",
#else
        "Generate SLURM job script for Savio",
#endif  // USE_MPI
        "Start without solving", "Test current game variant",
        "Game options",          "Solver options",
    };
    static ConstantReadOnlyString keys[] = {"s", "w", "t", "g", "o"};
    static const HookFunctionPointer hooks[] = {
#ifndef USE_MPI
        &SolveAndStart,
#else
        &InteractiveSavioPartitionSelect,
#endif  // USE_MPI
        &InteractivePostSolve,   &TestCurrentGameVariant,
        &InteractiveGameOptions, &InteractiveSolverOptions,
    };
    int num_items = sizeof(items) / sizeof(items[0]);

    return AutoMenu(title, num_items, items, keys, hooks, &UpdateTitle);
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
static int SolveAndStart(ReadOnlyString key) {
    // Auxiliary variable currently unused.
    int error = SolverManagerSolve(NULL);
    if (error != kNoError) {
        fprintf(stderr, "Solver manager failed to solve game\n");
        return 0;  // Go back to previous menu.
    }

    InteractiveMatchSetSolved(true);

    return InteractivePostSolve(key);
}
#endif  // USE_MPI

static int TestCurrentGameVariant(ReadOnlyString key) {
    (void)key;  // Unused.
    long seed;
    char input[100];
    printf(
        "Please enter a 64-bit integer as a seed, or leave blank to use a "
        "random seed based on current time: ");
    if (fgets(input, sizeof(input), stdin)) {
        // Attempt to parse a long from the input
        if (sscanf(input, "%ld", &seed) != 1) {
            // If parsing fails, use the current time as the seed
            seed = (long)time(NULL);
        }
    } else {
        // In case fgets fails, fallback to using the current time as the seed
        seed = (long)time(NULL);
    }
    printf("Testing with seed %ld\n", seed);
    int error = SolverManagerTest(seed);
    if (error != 0) {
        printf("\nTestCurrentGameVariant: an error occurred. Explanation: %s\n",
               SolverManagerExplainTestError(error));
    } else {
        puts(
            "\n****************************\n"
            "***** ALL TESTS PASSED *****\n"
            "****************************\n");
    }

    return 0;
}

static void UpdateTitle(void) {
    const Game *current_game = InteractiveMatchGetCurrentGame();
    int variant_index = InteractiveMatchGetVariantIndex();
    sprintf(title, title_format, current_game->formal_name, variant_index);
}

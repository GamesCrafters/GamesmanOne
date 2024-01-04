#include "core/headless/hanalyze.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // free

#include "core/gamesman_types.h"
#include "core/headless/hutils.h"
#include "core/misc.h"
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/solvers/solver_manager.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "games/game_manager.h"

static void *GenerateAnalyzeOptions(bool force, int verbose);

// -----------------------------------------------------------------------------

int HeadlessAnalyze(ReadOnlyString game_name, int variant_id,
                    ReadOnlyString data_path, bool force, int verbose) {
    int error = HeadlessInitSolver(game_name, variant_id, data_path);
    if (error != 0) return error;

    void *options = GenerateAnalyzeOptions(force, verbose);
    error = SolverManagerAnalyze(options);
    free(options);
    if (error != 0) {
        fprintf(stderr, "HeadlessAnalyze: analysis failed with code %d\n",
                error);
    }
    return error;
}

// -----------------------------------------------------------------------------

static void *GenerateAnalyzeOptions(bool force, int verbose) {
    const Game *game = GameManagerGetCurrentGame();
    assert(game != NULL);
    if (game->solver == &kRegularSolver) {
        RegularSolverAnalyzeOptions *options =
            (RegularSolverAnalyzeOptions *)SafeMalloc(
                sizeof(RegularSolverAnalyzeOptions));
        options->force = force;
        options->verbose = verbose;
        return (void *)options;
    } else if (game->solver == &kTierSolver) {
        TierSolverAnalyzeOptions *options =
            (TierSolverAnalyzeOptions *)SafeMalloc(
                sizeof(TierSolverAnalyzeOptions));
        options->force = force;
        options->verbose = verbose;
        return (void *)options;
    }
    NotReached("GenerateAnalyzeOptions: no valid solver found");
    return NULL;
}

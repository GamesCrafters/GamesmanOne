#include "core/headless/hsolve.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdio.h>    // printf, fprintf, stderr
#include <stdlib.h>   // free

#include "core/gamesman_types.h"
#include "core/headless/hutils.h"
#include "core/misc.h"
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/solvers/solver_manager.h"
#include "core/solvers/tier_solver/tier_solver.h"

static void *GenerateSolveOptions(ReadOnlyString game_name, bool force,
                                  int verbose);

// -----------------------------------------------------------------------------

int HeadlessSolve(ReadOnlyString game_name, int variant_id,
                  ReadOnlyString data_path, bool force, int verbose) {
    int error = HeadlessInitSolver(game_name, variant_id, data_path);
    if (error != 0) return error;

    void *options = GenerateSolveOptions(game_name, force, verbose);
    error = SolverManagerSolve(options);
    free(options);
    if (error != 0) {
        fprintf(stderr, "HeadlessSolve: solve failed with code %d\n", error);
    }
    return error;
}

// -----------------------------------------------------------------------------

static void *GenerateSolveOptions(ReadOnlyString game_name, bool force,
                                  int verbose) {
    const Game *game = HeadlessGetGame(game_name);
    if (game == NULL) {
        fprintf(stderr, "game [%s] not found\n", game_name);
        return NULL;
    }

    if (game->solver == &kRegularSolver) {
        RegularSolverSolveOptions *options =
            (RegularSolverSolveOptions *)SafeMalloc(
                sizeof(RegularSolverSolveOptions));
        options->force = force;
        options->verbose = verbose;
        return (void *)options;
    } else if (game->solver == &kTierSolver) {
        TierSolverSolveOptions *options = (TierSolverSolveOptions *)SafeMalloc(
            sizeof(TierSolverSolveOptions));
        options->force = force;
        options->verbose = verbose;
        return (void *)options;
    }  // Append new solvers to the end.
    
    NotReached("GenerateSolveOptions: no valid solver found");
    return NULL;
}

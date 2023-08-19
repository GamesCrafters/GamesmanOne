#include "core/solvers/solver_manager.h"

#include <assert.h>
#include <stddef.h>

#include "core/db/db_manager.h"
#include "core/gamesman_types.h"
#include "core/misc.h"

static const Solver *current_solver;

int SolverManagerInitSolver(const Game *game) {
    current_solver = game->solver;
    int error = current_solver->Init(game->solver_api);
    if (error != 0) return error;

    const GameVariant *variant = game->GetCurrentVariant();
    int variant_index = GameVariantToIndex(variant);
    error = DbManagerInitDb(current_solver, game->name, variant_index, NULL);
    if (error != 0) return error;
    
    return 0;
}

int SolverManagerGetSolverStatus(void) {
    assert(current_solver != NULL);
    return current_solver->GetStatus();
}

int SolverManagerSolve(void *aux) { return current_solver->Solve(aux); }

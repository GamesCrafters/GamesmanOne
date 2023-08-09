#include "core/solver_manager.h"

#include <assert.h>
#include <stddef.h>

static Solver *current_solver;

int SolverManagerInitSolver(const Game *game) {
    current_solver = game->solver;
    current_solver->Init(game->solver_api);
}

int SolverManagerGetSolverStatus(void) {
    assert(current_solver != NULL);
    return current_solver->GetStatus();
}

int SolverManagerSolve(void *aux) {
    current_solver->Solve(aux);
}

#include "core/solver_manager.h"

#include <assert.h>
#include <stddef.h>

#include "core/db_manager.h"
#include "core/gamesman_types.h"

static Solver *current_solver;

int SolverManagerInitSolver(const Game *game) {
    current_solver = game->solver;
    int result = current_solver->Init(game->solver_api);
    if (result != 0) return result;
    result = DbManagerInitDb(current_solver);
    if (result != 0) return result;
    return 0;
}

int SolverManagerGetSolverStatus(void) {
    assert(current_solver != NULL);
    return current_solver->GetStatus();
}

int SolverManagerSolve(void *aux) { current_solver->Solve(aux); }

#ifndef GAMESMANEXPERIMENT_CORE_SOLVER_MANAGER_H_
#define GAMESMANEXPERIMENT_CORE_SOLVER_MANAGER_H_

#include "core/gamesman_types.h"

int SolverManagerInitSolver(const Game *game);
int SolverManagerGetSolverStatus(void);
int SolverManagerSolve(void *aux);

#endif  // GAMESMANEXPERIMENT_CORE_SOLVER_MANAGER_H_

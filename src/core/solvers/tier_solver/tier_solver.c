#include "core/solvers/tier_solver/tier_solver.h"

#include "core/db/naivedb/naivedb.h"
#include "core/gamesman_types.h"

static int TierSolverInit(const void *solver_api);
static int TierSolverSolve(void *aux);
static int TierSolverGetStatus(void);
static const SolverConfiguration *TierSolverGetCurrentConfiguration(void);
static int TierSolverSetOption(int option, int selection);

Solver kTierSolver = {
    .name = "Tier Solver",
    .db = &kNaiveDb,

    .Init = &TierSolverInit,
    .Solve = &TierSolverSolve,
    .GetStatus = &TierSolverGetStatus,
    
    .GetCurrentConfiguration = &TierSolverGetCurrentConfiguration,
    .SetOption = &TierSolverSetOption
};

static int TierSolverInit(const void *solver_api) {

}

static int TierSolverSolve(void *aux) {

}

static int TierSolverGetStatus(void) {

}

static const SolverConfiguration *TierSolverGetCurrentConfiguration(void) {

}

static int TierSolverSetOption(int option, int selection) {

}

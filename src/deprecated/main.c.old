#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "core/gamesman.h"
#include "core/generic_hash/generic_hash.h"
#include "core/solver.h"
#include "games/mttt/mttt.h"
#include "games/mtttier/mtttier.h"

int main(void) {
    // This should be in a DbInit function.
    AnalysisInit(&global_analysis);

    // These should be in a MainMenu function.
    memset(&regular_solver, 0, sizeof(regular_solver));
    memset(&tier_solver, 0, sizeof(tier_solver));
    GenericHashReinitialize();
    MtttierInit();
    SolverSolve(false);

    return 0;
}

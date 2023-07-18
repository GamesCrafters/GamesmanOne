#include <stdbool.h>
#include <stdio.h>

#include "core/gamesman.h"
#include "core/solver.h"
#include "games/mttt/mttt.h"

int main(void) {
    // This should be in a DbInit function.
    AnalysisInit(&global_analysis);

    // These should be in a MainMenu function.
    MtttInitialize();
    SolverSolve(false);

    return 0;
}

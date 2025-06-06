#include "core/interactive/games/presolve/postsolve/analyze/analyze.h"

#include <stdio.h>  // fprintf, stderr

#include "core/interactive/automenu.h"
#include "core/solvers/solver_manager.h"
#include "core/types/gamesman_types.h"

int InteractiveAnalyze(ReadOnlyString key) {
    (void)key;  // Unused.

    // Auxiliary variable currently unused.
    int error = SolverManagerAnalyze(NULL);
    if (error != 0) {
        fprintf(stderr, "InteractiveAnalyze: failed with code %d\n", error);
    }

    return 0;
}

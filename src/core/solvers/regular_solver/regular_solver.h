#ifndef GAMESMANEXPERIMENT_CORE_SOLVERS_REGULAR_SOLVER_REGULAR_SOLVER_H_
#define GAMESMANEXPERIMENT_CORE_SOLVERS_REGULAR_SOLVER_REGULAR_SOLVER_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/gamesman_types.h"

extern const Solver kRegularSolver;

typedef struct RegularSolverApi {
    int64_t (*GetNumPositions)(void);
    Position (*GetInitialPosition)(void);

    MoveArray (*GenerateMoves)(Position position);
    Value (*Primitive)(Position position);
    Position (*DoMove)(Position position, Move move);
    bool (*IsLegalPosition)(Position position);
    Position (*GetCanonicalPosition)(Position position);
    int (*GetNumberOfCanonicalChildPositions)(Position position);
    PositionArray (*GetCanonicalChildPositions)(Position position);
    PositionArray (*GetCanonicalParentPositions)(Position position);
} RegularSolverApi;

#endif  // GAMESMANEXPERIMENT_CORE_SOLVERS_REGULAR_SOLVER_REGULAR_SOLVER_H_

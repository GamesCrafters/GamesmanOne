#ifndef GAMESMANONE_CORE_TYPES_SOLVER_OPTION_H
#define GAMESMANONE_CORE_TYPES_SOLVER_OPTION_H

#include "core/types/base.h"

enum SolverOptionConstants {
    kSolverOptionNameLengthMax = 63,
    kSolverOptionChoiceNameLengthMax = 127,
};

/** @brief Solver option for display in GAMESMAN interactive mode. */
typedef struct SolverOption {
    /** Human-readable name of the option. */
    char name[kSolverOptionNameLengthMax + 1];

    /** Number of choices associated with the option. */
    int num_choices;

    /** An array of strings, where each string is the name of a choice. Length =
     * num_choices.  */
    const ConstantReadOnlyString *choices;
} SolverOption;

#endif  // GAMESMANONE_CORE_TYPES_SOLVER_OPTION_H

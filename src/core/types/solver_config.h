#ifndef GAMESMANONE_CORE_TYPES_SOLVER_CONFIG_H
#define GAMESMANONE_CORE_TYPES_SOLVER_CONFIG_H

#include "core/types/solver_option.h"

/** @brief Solver configuration as an array of selected solver options. */
typedef struct SolverConfig {
    /**
     * Zero-terminated array of solver options. The user of this struct must
     * make sure that the last item in this array is completely zeroed out.
     */
    const SolverOption *options;

    /**
     * Array of selected choice indices to each option. Zero-terminated and
     * aligned to the options array (same number of options and selections.)
     */
    const int *selections;
} SolverConfig;

#endif  // GAMESMANONE_CORE_TYPES_SOLVER_CONFIG_H

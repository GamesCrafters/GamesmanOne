#ifndef GAMESMANONE_CORE_TYPES_SOLVER_H
#define GAMESMANONE_CORE_TYPES_SOLVER_H

#include "core/types/base.h"
#include "core/types/solver_config.h"

enum SolverConstants {
    kSolverNameLengthMax = 63,
};

/**
 * @brief Generic Solver type.
 * @note To implement a new Solver module, set the name of the new Solver and
 * each member function pointer to a module-specific function.
 *
 * @note A Solver can either be a regular solver or a tier solver. The actual
 * behavior and requirements of the solver is decided by the Solver and
 * reflected on its SOLVER_API, which is a custom struct defined in the Solver
 * module and implemented by the game developer. The game developer should
 * decide which Solver to use and implement its required Solver API functions.
 */
typedef struct Solver {
    /** Human-readable name of the solver. */
    char name[kSolverNameLengthMax + 1];

    /**
     * @brief Initializes the Solver.
     *
     * @param game_name Game name used internally by Gamesman.
     * @param variant Index of the current game variant.
     * @param solver_api Pointer to a struct that contains the implemented
     * Solver API functions. The game developer is responsible for using the
     * correct type of Solver API that applies to the current Solver,
     * implementing and setting up required API functions, and casting the
     * pointer to a constant generic pointer (const void *) type.
     * @param data_path Absolute or relative path to the data directory if
     * non-NULL. The default path "data" will be used if set to NULL.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Init)(ReadOnlyString game_name, int variant, const void *solver_api,
                ReadOnlyString data_path);

    /**
     * @brief Finalizes the Solver, freeing all allocated memory.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Finalize)(void);

    /**
     * @brief Solves the current game and stores the result if a Database is set
     * for the current Solver.
     *
     * @param aux Auxiliary parameter.
     */
    int (*Solve)(void *aux);

    /**
     * @brief Analyzes the current game.
     *
     * @param aux Auxiliary parameter.
     */
    int (*Analyze)(void *aux);

    /**
     * @brief Returns the solving status of the current game.
     *
     * @return Status code as defined by the actual Solver module.
     */
    int (*GetStatus)(void);

    /** @brief Returns the current configuration of this Solver. */
    const SolverConfig *(*GetCurrentConfig)(void);

    /**
     * @brief Sets the solver option with index OPTION to the choice of index
     * SELECTION.
     *
     * @param option Solver option index.
     * @param selection Choice index to select.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*SetOption)(int option, int selection);

    /**
     * @brief Probes and returns the value of the given TIER_POSITION. Results
     * in undefined behavior if the TIER_POSITION has not been solved, or if
     * TIER_POSITION is invalid or unreachable.
     *
     * @param tier_position Tier position to probe.
     *
     * @return Value of TIER_POSITION.
     */
    Value (*GetValue)(TierPosition tier_position);

    /**
     * @brief Probes and returns the remoteness of the given TIER_POSITION.
     * Results in undefined behavior if the TIER_POSITION has not been solved,
     * or if TIER_POSITION is invalid or unreachable.
     *
     * @param tier_position Tier position to probe.
     *
     * @return Remoteness of TIER_POSITION.
     */
    int (*GetRemoteness)(TierPosition tier_position);
} Solver;

#endif

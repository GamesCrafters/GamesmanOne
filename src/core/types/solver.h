#ifndef GAMESMANONE_CORE_TYPES_SOLVER_H
#define GAMESMANONE_CORE_TYPES_SOLVER_H

#include "core/types/base.h"
#include "core/types/solver_config.h"

enum SolverConstants {
    kSolverNameLengthMax = 63,
};

/**
 * @brief Generic Solver type.
 * @note To implement a new Solver module, correctly set the name of the new
 * Solver and set each member function pointer to a module-specific function.
 *
 * @note A Solver can either be a regular solver or a tier solver. The actual
 * behavior and requirements of the solver is decided by the Solver and
 * reflected on its SOLVER_API, which is a custom struct defined in the Solver
 * module and implemented by the game developer. Therefore, the game developer
 * must decide which Solver to use and implement the required Solver API
 * functions.
 */
typedef struct Solver {
    /** Human-readable name of the solver. */
    char name[kSolverNameLengthMax + 1];

    /**
     * @brief Initializes the Solver.
     * @note The user is responsible for passing the correct solver API that
     * appies to the current Solver. Passing NULL or an incompatible Solver API
     * results in undefined behavior.
     *
     * @param game_name Internal name of the game.
     * @param variant Index of the current game variant.
     * @param solver_api Pointer to a struct that contains the implemented
     * Solver API functions. The game developer is responsible for using the
     * correct type of Solver API that applies to the current Solver,
     * implementing and setting up required API functions, and casting the
     * pointer to a generic constant pointer (const void *) type.
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
} Solver;

#endif

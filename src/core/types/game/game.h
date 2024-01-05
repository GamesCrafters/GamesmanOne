#ifndef GAMESMANONE_CORE_TYPES_GAME_H
#define GAMESMANONE_CORE_TYPES_GAME_H

#include "core/types/game/game_variant.h"
#include "core/types/gameplay_api/gameplay_api.h"
#include "core/types/solver.h"
#include "core/types/universal_web_api.h"

enum GameConstants {
    kGameNameLengthMax = 31,
    kGameFormalNameLengthMax = 127,
};

/**
 * @brief Generic Game type.
 *
 * @details A game should have an internal name, a human-readable formal name
 * for textUI display, a Solver to use, a set of implemented API functions for
 * the chosen Solver, a set of implemented API functions for the game play
 * system, functions to initialize and finalize the game module, and functions
 * to get/set the current game variant. The solver interface is required for
 * solving the game. The gameplay interface is required for textUI play loop
 * and debugging. The game variant interface is optional, and may be set to
 * NULL if there is only one variant.
 */
typedef struct Game {
    /** Internal name of the game, must contain no white spaces or special
     * characters. */
    char name[kGameNameLengthMax + 1];

    /** Human-readable name of the game. */
    char formal_name[kGameFormalNameLengthMax + 1];

    /** Solver to use. */
    const Solver *solver;

    /** Pointer to an object containing implemented API functions for the
     * selected Solver. */
    const void *solver_api;

    /** Pointer to a GameplayApi object which contains implemented gameplay API
     * functions. */
    const GameplayApi *gameplay_api;

    const UniversalWebApi *uwapi;

    /**
     * @brief Initializes the game module.
     *
     * @param aux Auxiliary parameter.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Init)(void *aux);

    /**
     * @brief Finalizes the game module, freeing all allocated memory.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Finalize)(void);

    /**
     * @brief Returns the current variant of the game as a read-only GameVariant
     * object.
     */
    const GameVariant *(*GetCurrentVariant)(void);

    /**
     * @brief Sets the game variant option with index OPTION to the choice of
     * index SELECTION.
     *
     * @param option Index of the option to modify.
     * @param selection Index of the choice to select.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*SetVariantOption)(int option, int selection);
} Game;

#endif  // GAMESMANONE_CORE_TYPES_GAME_H

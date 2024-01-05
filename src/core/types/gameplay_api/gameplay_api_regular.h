#ifndef GAMESMANONE_CORE_TYPES_GAMEPLAY_API_REGULAR_H
#define GAMESMANONE_CORE_TYPES_GAMEPLAY_API_REGULAR_H

#include "core/types/base.h"
#include "core/types/move_array.h"

typedef struct GameplayApiRegular {
    /**
     * @brief Converts POSITION into a position string and stores it in BUFFER.
     *
     * @note Assumes that POSITION is valid and BUFFER has enough space to hold
     * the position string. Results in undefined behavior otherwise.
     */
    int (*PositionToString)(Position position, char *buffer);

    /**
     * @brief Returns an array of available moves at the given POSITION.
     *
     * @note Assumes that POSITION is valid. Results in undefined behavior
     * otherwise.
     */
    MoveArray (*GenerateMoves)(Position position);

    /**
     * @brief Returns the resulting position after performing the given MOVE at
     * the given POSITION.
     *
     * @note Assumes that POSITION is valid and MOVE is a valid move at
     * POSITION. Passing an illegal position or an illegal move results in
     * undefined behavior.
     */
    Position (*DoMove)(Position position, Move move);

    /**
     * @brief Returns the value of the given POSITION if it is primitive, or
     * kUndecided otherwise.
     *
     * @note Assumes POSITION is valid. Passing an illegal position results in
     * undefined behavior.
     */
    Value (*Primitive)(Position position);

    /**
     * @brief Returns the canonical position that is symmetric to POSITION.
     *
     * @details By convention, a canonical position is one with the smallest
     * hash value in a set of symmetrical positions. For each position[i] within
     * the set including the canonical position itself, calling
     * GetCanonicalPosition on position[i] returns the canonical position.
     *
     * @note Assumes POSITION is valid. Passing an illegal position to this
     * function results in undefined behavior.
     *
     * @note Required for NON-TIER games ONLY IF position symmetry removal
     * optimization was used to solve the game. Set to NULL otherwise.
     * Inconsistent usage of this function between solving and gameplay results
     * in undefined behavior.
     */
    Position (*GetCanonicalPosition)(Position position);
} GameplayApiRegular;

#endif  // GAMESMANONE_CORE_TYPES_GAMEPLAY_API_REGULAR_H

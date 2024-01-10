#ifndef GAMESMANONE_CORE_TYPES_GAMEPLAY_API_TIER_H
#define GAMESMANONE_CORE_TYPES_GAMEPLAY_API_TIER_H

#include "core/types/base.h"
#include "core/types/move_array.h"

typedef struct GameplayApiTier {
    /**
     * @brief Returns the tier in which the initial position belongs to.
     * Different game variants may have different initial tiers.
     */
    Tier (*GetInitialTier)(void);

    /**
     * @brief Converts TIER_POSITION into a position string and stores it in
     * BUFFER.
     *
     * @note Assumes that TIER_POSITION is valid and BUFFER has enough space to
     * hold the position string. Results in undefined behavior otherwise.
     */
    int (*TierPositionToString)(TierPosition tier_position, char *buffer);

    /**
     * @brief Returns an array of available moves at the given TIER_POSITION.
     *
     * @note Assumes that TIER_POSITION is valid. Results in undefined behavior
     * otherwise.
     */
    MoveArray (*GenerateMoves)(TierPosition tier_position);

    /**
     * @brief Returns the resulting tier position after performing the given
     * MOVE at the given TIER_POSITION.
     *
     * @note Assumes that TIER_POSITION is valid and MOVE is a valid move at
     * TIER_POSITION. Passing an invalid tier, an illegal position within the
     * tier, or an illegal move results in undefined behavior.
     */
    TierPosition (*DoMove)(TierPosition tier_position, Move move);

    /**
     * @brief Returns the value of the given TIER_POSITION if it is primitive,
     * or kUndecided otherwise.
     *
     * @note Assumes TIER_POSITION is valid. Passing an invalid tier or an
     * illegal position within the tier results in undefined behavior.
     */
    Value (*Primitive)(TierPosition tier_position);
} GameplayApiTier;

#endif  // GAMESMANONE_CORE_TYPES_GAMEPLAY_API_TIER_H

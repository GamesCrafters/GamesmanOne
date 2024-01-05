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
    MoveArray (*TierGenerateMoves)(TierPosition tier_position);

    /**
     * @brief Returns the resulting tier position after performing the given
     * MOVE at the given TIER_POSITION.
     *
     * @note Assumes that TIER_POSITION is valid and MOVE is a valid move at
     * TIER_POSITION. Passing an invalid tier, an illegal position within the
     * tier, or an illegal move results in undefined behavior.
     */
    TierPosition (*TierDoMove)(TierPosition tier_position, Move move);

    /**
     * @brief Returns the value of the given TIER_POSITION if it is primitive,
     * or kUndecided otherwise.
     *
     * @note Assumes TIER_POSITION is valid. Passing an invalid tier or an
     * illegal position within the tier results in undefined behavior.
     */
    Value (*TierPrimitive)(TierPosition tier_position);

    /**
     * @brief Returns the canonical position within the same tier that is
     * symmetric to TIER_POSITION.
     *
     * @details GAMESMAN currently does not support position symmetry removal
     * across tiers. By convention, a canonical position is one with the
     * smallest hash value in a set of symmetrical positions which all belong to
     * the same tier. For each position[i] within the set including the
     * canonical position itself, calling GetCanonicalPosition on position[i]
     * returns the canonical position.
     *
     * @note Assumes TIER_POSITION is valid. Passing an invalid tier or an
     * illegal position within the tier to this function results in undefined
     * behavior.
     *
     * @note Required for TIER games ONLY IF position symmetry removal
     * optimization was used to solve the game. Set to NULL otherwise.
     * Inconsistent usage of this function between solving and gameplay results
     * in undefined behavior.
     */
    Position (*TierGetCanonicalPosition)(TierPosition tier_position);

    /**
     * @brief Returns the canonical tier symmetric to TIER. Returns TIER if TIER
     * itself is canonical.
     *
     * @details By convention, a canonical tier is one with the smallest tier
     * value in a set of symmetrical tiers. For each tier[i] within the set
     * including the canonical tier itself, calling GetCanonicalTier(tier[i])
     * returns the canonical tier.
     *
     * @note Assumes TIER is valid. Passing an invalid
     * tier to this function results in undefined behavior.
     *
     * @note Required for TIER games ONLY IF tier symmetry removal optimization
     * was used to solve the game. Set to NULL otherwise. Inconsistent usage of
     * this function between solving and gameplay results in undefined behavior.
     */
    Tier (*GetCanonicalTier)(Tier tier);

    /**
     * @brief Returns the position, which is symmetric to the given
     * TIER_POSITION, in SYMMETRIC tier.
     *
     * @note Assumes both TIER_POSITION and SYMMETRIC are valid. Furthermore,
     * assumes that the tier as specified by TIER_POSITION is symmetric to the
     * SYMMETRIC tier; that is, calling GetCanonicalTier(tier_position.tier)
     * and calling GetCanonicalTier(symmetric) return the same canonical tier.
     *
     * @note Required for TIER games ONLY IF tier symmetry removal optimization
     * was used to solve the game. Set to NULL otherwise. Inconsistent usage of
     * this function between solving and gameplay results in undefined behavior.
     */
    Position (*GetPositionInSymmetricTier)(TierPosition tier_position,
                                           Tier symmetric);
} GameplayApiTier;

#endif  // GAMESMANONE_CORE_TYPES_GAMEPLAY_API_TIER_H

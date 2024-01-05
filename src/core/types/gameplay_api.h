#ifndef GAMESMANONE_CORE_TYPES_GAMEPLAY_API_H
#define GAMESMANONE_CORE_TYPES_GAMEPLAY_API_H

#include "core/types/base.h"
#include "core/types/move_array.h"

/**
 * @brief GAMESMAN interactive game play API.
 *
 * @details There are two sets of APIs, one for tier games and one for non-tier
 * games. The game developer should implement exactly one of the two APIs and
 * set all irrelavent fields to zero (0) or NULL. If neither is fully
 * implemented, the game will be rejected by the gameplay system. Implementing
 * both APIs results in undefined behavior.
 */
typedef struct GameplayApi {
    /**
     * @brief Returns the tier in which the initial position belongs to.
     * Different game variants may have different initial tiers.
     *
     * @note Required for TIER games. The system will not recognize the game
     * as a tier game if this function is not implemented.
     */
    Tier (*GetInitialTier)(void);

    /**
     * @brief If the game is a tier game, returns the initial position inside
     * the initial tier. Otherwise, returns the initial position. Different game
     * variants may have different initial positions.
     *
     * @note Required for ALL games. The gameplay system will reject the game if
     * this function is not implemented.
     */
    Position (*GetInitialPosition)(void);

    /**
     * @brief Maximum length of a position string not including the
     * NULL-terminating character ('\0') as a constant integer.
     *
     * @details The gameplay system will try to allocate this amount of space
     * plus one additional byte for the NULL-terminating character ('\0') as
     * buffer for position strings. It is the game developer's responsibility to
     * precalculate this value and make sure that enough space is provided.
     *
     * @note Required for ALL games. The gameplay system will reject the game if
     * this function is not implemented.
     */
    int position_string_length_max;

    /**
     * @brief Converts POSITION into a position string and stores it in BUFFER.
     *
     * @note Assumes that POSITION is valid and BUFFER has enough space to hold
     * the position string. Results in undefined behavior otherwise.
     *
     * @note Required for NON-TIER games. The system will not recognize the
     * game as a non-tier game if this function is not implemented.
     */
    int (*PositionToString)(Position position, char *buffer);

    /**
     * @brief Converts TIER_POSITION into a position string and stores it in
     * BUFFER.
     *
     * @note Assumes that TIER_POSITION is valid and BUFFER has enough space to
     * hold the position string. Results in undefined behavior otherwise.
     *
     * @note Required for TIER games. The system will not recognize the game
     * as a tier game if this function is not implemented.
     */
    int (*TierPositionToString)(TierPosition tier_position, char *buffer);

    /**
     * @brief Maximum length of a move string not including the NULL-terminating
     * character ('\0') as a constant integer.
     *
     * @details The gameplay system will try to allocate this amount of space
     * plus one additional byte for the NULL-terminating character ('\0') as
     * buffer for move strings. It is the game developer's responsibility to
     * precalculate this value and make sure that enough space is provided.
     *
     * @note Required for ALL games. The gameplay system will reject the game if
     * this function is not implemented.
     */
    int move_string_length_max;

    /**
     * @brief Converts MOVE into a move string and stores it in BUFFER.
     *
     * @note Assumes that MOVE is valid and BUFFER has enough space to hold the
     * move string. Results in undefined behavior otherwise.
     *
     * @note Required for ALL games. The gameplay system will reject the game if
     * this function is not implemented.
     */
    int (*MoveToString)(Move move, char *buffer);

    /**
     * @brief Returns true if the given MOVE_STRING is recognized as a valid
     * move string for the current game, or false otherwise.
     *
     * @param move_string User (typically the user of GAMESMAN interactive
     * through the text user interface) provided move string to be validated.
     *
     * @note Required for ALL games. The gameplay system will reject the game if
     * this function is not implemented.
     */
    bool (*IsValidMoveString)(ReadOnlyString move_string);

    /**
     * @brief Converts the MOVE_STRING to a Move and returns it.
     *
     * @note Assmues that the given MOVE_STRING is valid. Results in undefined
     * behavior otherwise. Therefore, it is the developer of the gameplay
     * system's responsibility to validate the MOVE_STRING using the
     * IsValidMoveString() function before calling this function.
     *
     * @note Required for ALL games. The gameplay system will reject the game if
     * this function is not implemented.
     */
    Move (*StringToMove)(ReadOnlyString move_string);

    /**
     * @brief Returns an array of available moves at the given POSITION.
     *
     * @note Assumes that POSITION is valid. Results in undefined behavior
     * otherwise.
     *
     * @note Required for NON-TIER games. The system will not recognize the game
     * as a non-tier game if this function is not implemented.
     */
    MoveArray (*GenerateMoves)(Position position);

    /**
     * @brief Returns an array of available moves at the given TIER_POSITION.
     *
     * @note Assumes that TIER_POSITION is valid. Results in undefined behavior
     * otherwise.
     *
     * @note Required for TIER games. The system will not recognize the game
     * as a tier game if this function is not implemented.
     */
    MoveArray (*TierGenerateMoves)(TierPosition tier_position);

    /**
     * @brief Returns the resulting position after performing the given MOVE at
     * the given POSITION.
     *
     * @note Assumes that POSITION is valid and MOVE is a valid move at
     * POSITION. Passing an illegal position or an illegal move results in
     * undefined behavior.
     *
     * @note Required for NON-TIER games. The system will not recognize the game
     * as a non-tier game if this function is not implemented.
     */
    Position (*DoMove)(Position position, Move move);

    /**
     * @brief Returns the resulting tier position after performing the given
     * MOVE at the given TIER_POSITION.
     *
     * @note Assumes that TIER_POSITION is valid and MOVE is a valid move at
     * TIER_POSITION. Passing an invalid tier, an illegal position within the
     * tier, or an illegal move results in undefined behavior.
     *
     * @note Required for TIER games. The system will not recognize the game
     * as a tier game if this function is not implemented.
     */
    TierPosition (*TierDoMove)(TierPosition tier_position, Move move);

    /**
     * @brief Returns the value of the given POSITION if it is primitive, or
     * kUndecided otherwise.
     *
     * @note Assumes POSITION is valid. Passing an illegal position results in
     * undefined behavior.
     *
     * @note Required for NON-TIER games. The system will not recognize the game
     * as a non-tier game if this function is not implemented.
     */
    Value (*Primitive)(Position position);

    /**
     * @brief Returns the value of the given TIER_POSITION if it is primitive,
     * or kUndecided otherwise.
     *
     * @note Assumes TIER_POSITION is valid. Passing an invalid tier or an
     * illegal position within the tier results in undefined behavior.
     *
     * @note Required for TIER games. The system will not recognize the game
     * as a tier game if this function is not implemented.
     */
    Value (*TierPrimitive)(TierPosition tier_position);

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
} GameplayApi;

#endif  // GAMESMANONE_CORE_TYPES_GAMEPLAY_API_H

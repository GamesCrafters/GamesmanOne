#ifndef GAMESMANONE_CORE_TYPES_GAMEPLAY_API_COMMON_H
#define GAMESMANONE_CORE_TYPES_GAMEPLAY_API_COMMON_H

#include "core/types/base.h"

typedef struct GameplayApiCommon {
    /**
     * @brief If the game is a tier game, returns the initial position inside
     * the initial tier. Otherwise, returns the initial position. Different game
     * variants may have different initial positions.
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
     */
    int position_string_length_max;

    /**
     * @brief Maximum length of a move string not including the NULL-terminating
     * character ('\0') as a constant integer.
     *
     * @details The gameplay system will try to allocate this amount of space
     * plus one additional byte for the NULL-terminating character ('\0') as
     * buffer for move strings. It is the game developer's responsibility to
     * precalculate this value and make sure that enough space is provided.
     */
    int move_string_length_max;

    /**
     * @brief Converts MOVE into a move string and stores it in BUFFER.
     *
     * @note Assumes that MOVE is valid and BUFFER has enough space to hold the
     * move string. Results in undefined behavior otherwise.
     */
    int (*MoveToString)(Move move, char *buffer);

    /**
     * @brief Returns true if the given MOVE_STRING is recognized as a valid
     * move string for the current game, or false otherwise.
     *
     * @param move_string User(typically the user of GAMESMAN interactive
     * through the text user interface)-provided move string to be validated.
     */
    bool (*IsValidMoveString)(ReadOnlyString move_string);

    /**
     * @brief Converts the MOVE_STRING to a Move and returns it.
     *
     * @note Assmues that the given MOVE_STRING is valid. Results in undefined
     * behavior otherwise. Therefore, it is the developer of the gameplay
     * system's responsibility to validate the MOVE_STRING using the
     * IsValidMoveString() function before calling this function.
     */
    Move (*StringToMove)(ReadOnlyString move_string);
} GameplayApiCommon;

#endif  // GAMESMANONE_CORE_TYPES_GAMEPLAY_API_COMMON_H

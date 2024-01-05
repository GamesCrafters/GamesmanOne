#ifndef GAMESMANONE_CORE_TYPES_BASE_H
#define GAMESMANONE_CORE_TYPES_BASE_H

#include <stdint.h>  // int64_t

/** @brief Tier as a 64-bit integer. */
typedef int64_t Tier;

/** @brief Game position as a 64-bit integer hash. */
typedef int64_t Position;

/** @brief Game move as a 64-bit integer. */
typedef int64_t Move;

/**
 * @brief Tier and Position. In Tier games, a position is uniquely identified by
 * the Tier it belongs and its Position hash inside that tier.
 */
typedef struct TierPosition {
    Tier tier;
    Position position;
} TierPosition;

/**
 * @brief Possible values of a game position.
 *
 * @note Always make sure that kUndecided is 0 as other components rely on this
 * assumption.
 */
typedef enum Value {
    kErrorValue = -1,
    kUndecided = 0,
    kLose,
    kDraw,
    kTie,
    kWin,
} Value;

/**
 * @brief Pointer to a read-only string cannot be used to modify the contents of
 * the string.
 */
typedef const char *ReadOnlyString;

/**
 * @brief Constant pointer to a read-only string. The pointer is constant, which
 * means its value cannot be changed once it is initialized. The string is
 * read-only, which means that the pointer cannot be used to modify the contents
 * of the string.
 */
typedef const ReadOnlyString ConstantReadOnlyString;

#endif  // GAMESMANONE_CORE_TYPES_BASE_H

#ifndef GAMESMANONE_CORE_TYPES_GAME_VARIANT_OPTION_H
#define GAMESMANONE_CORE_TYPES_GAME_VARIANT_OPTION_H

#include "core/types/base.h"

enum GameVariantOptionConstants {
    kGameVariantOptionNameMax = 63,
};

/** @brief Game variant option for display in GAMESMAN interactive mode. */
typedef struct GameVariantOption {
    /** Human-readable name of the option. */
    char name[kGameVariantOptionNameMax + 1];

    /** Number of choices associate with the option. */
    int num_choices;

    /** An array of strings, where each string is a name of a choice. Length =
     * num_choices.  */
    const ConstantReadOnlyString *choices;
} GameVariantOption;

#endif  // GAMESMANONE_CORE_TYPES_GAME_VARIANT_OPTION_H

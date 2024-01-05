#ifndef GAMESMANONE_CORE_TYPES_GAME_VARIANT_H
#define GAMESMANONE_CORE_TYPES_GAME_VARIANT_H

#include "core/data_structures/int64_array.h"
#include "core/types/game_variant_option.h"

/**
 * @brief Game variant as an array of selected variant options.
 *
 * @details A game variant is determined by a set of variant options. Each
 * variant option decides some aspect of the game rule. The game developer is
 * responsible for providing the possible choices for each one of the variant
 * options as strings (see GameVariantOption::choices). The user of GAMESMAN
 * interactive can then set the variant by selecting a value for each option
 * using the game-specific SetVariantOption method.
 *
 * @example A Tic-Tac-Toe game can be generalized and played on a M by N board
 * with a goal of connecting K pieces in a row. Then, we can have three game
 * variant options "dimension M", "dimension N", and "number of pieces to
 * connect (K)." A board too small can make the game less interesting, whereas a
 * board too large can render the game unsolvable. Therefore, the game developer
 * decides to allow M, N, and K to be all within the range [2, 5], and sets the
 * corresponding choices to {"2", "3", "4", "5"}, for each one of the three
 * GameVariantOptions.
 */
typedef struct GameVariant {
    /**
     * Zero-terminated array of game variant options. The user of this struct
     * must make sure that the last item in this array is completely zeroed out.
     */
    const GameVariantOption *options;

    /**
     * Array of selected choice indices to each option. Zero-terminated and
     * aligned to the options array (same number of options and selections.)
     */
    const int *selections;
} GameVariant;

int GameVariantGetNumOptions(const GameVariant *variant);
int GameVariantToIndex(const GameVariant *variant);
Int64Array VariantIndexToSelections(int index, const GameVariant *variant);

#endif  // GAMESMANONE_CORE_TYPES_GAME_VARIANT_H

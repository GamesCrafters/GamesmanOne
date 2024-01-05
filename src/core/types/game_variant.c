#include "core/types/game_variant.h"

#include <stddef.h>  // NULL

#include "core/data_structures/int64_array.h"
#include "core/types/game_variant_option.h"

int GameVariantGetNumOptions(const GameVariant *variant) {
    int ret = 0;
    while (variant->options[ret].num_choices > 0) {
        ++ret;
    }
    return ret;
}

int GameVariantToIndex(const GameVariant *variant) {
    if (variant == NULL) return 0;

    int ret = 0;
    for (int i = 0; variant->options[i].num_choices > 0; ++i) {
        ret = ret * variant->options[i].num_choices + variant->selections[i];
    }

    return ret;
}

Int64Array VariantIndexToSelections(int index, const GameVariant *variant) {
    int num_options = GameVariantGetNumOptions(variant);
    Int64Array ret;
    Int64ArrayInit(&ret);
    if (!Int64ArrayResize(&ret, num_options)) return ret;

    for (int i = num_options; i >= 0; --i) {
        int selection = index % variant->options[i].num_choices;
        ret.array[i] = selection;
        index /= variant->options[i].num_choices;
    }

    return ret;
}

#include "core/interactive/games/presolve/options/options.h"

#include <stddef.h>  // NULL
#include <stdio.h>   // sprintf
#include <stdlib.h>  // free
#include <string.h>  // strcat, strncat, strlen

#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/match.h"
#include "core/interactive/games/presolve/options/choices/choices.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"

static char **items;

static int GetNumOptions(const GameVariant *variant);

static void UpdateItems(void);
static char **AllocateItems(int num_items, int item_length);
static char **AllocateKeys(int num_items);
static HookFunctionPointer *AllocateHooks(int num_items);

static void FreeAll(int num_items, char **keys, HookFunctionPointer *hooks);

void InteractiveGameOptions(ReadOnlyString key) {
    (void)key;  // Unused.
    const Game *current_game = InteractiveMatchGetCurrentGame();
    const GameVariant *variant = InteractiveMatchGetVariant();
    if (variant == NULL) {
        printf(
            "The game has only one variant and therefore no options are "
            "available.\n");
        return;
    }

    // Hard-coded size based on the title definition below.
    char title[29 + kGameFormalNameLengthMax];
    sprintf(title, "Game-specific options for %s", current_game->formal_name);

    int num_items = GetNumOptions(variant);
    UpdateItems();
    char **keys = AllocateKeys(num_items);
    HookFunctionPointer *hooks = AllocateHooks(num_items);
    for (int i = 0; i < num_items; ++i) {
        snprintf(keys[i], kKeyLengthMax, "%d", i);
        hooks[i] = &InteractiveGameOptionChoices;
    }

    AutoMenu(title, num_items, (ConstantReadOnlyString *)items,
             (ConstantReadOnlyString *)keys, hooks, &UpdateItems);
    FreeAll(num_items, keys, hooks);
}

static int GetNumOptions(const GameVariant *variant) {
    int ret = 0;
    while (variant->options[ret].choices != NULL) {
        ++ret;
    }
    return ret;
}

static void UpdateItems(void) {
    // item: "Change option [<option_name>] (currently <selection>)"
    static ConstantReadOnlyString prefix = "Change option [";
    static ConstantReadOnlyString middle = "] (currently ";
    static ConstantReadOnlyString postfix = ")";
    int item_length = strlen(prefix) + kGameVariantOptionNameMax;
    item_length += strlen(middle) + kSolverOptionChoiceNameLengthMax;
    item_length += strlen(postfix) + 1;
    const GameVariant *variant = InteractiveMatchGetVariant();
    int num_items = GetNumOptions(variant);
    if (items == NULL) items = AllocateItems(num_items, item_length);

    for (int i = 0; i < num_items; ++i) {
        items[i][0] = '\0';
        strcat(items[i], prefix);
        strcat(items[i], variant->options[i].name);
        strcat(items[i], middle);
        int selection = variant->selections[i];
        strncat(items[i], variant->options[i].choices[selection],
                kSolverOptionChoiceNameLengthMax);
        strcat(items[i], postfix);
    }
}

static char **AllocateItems(int num_items, int item_length) {
    char **items = (char **)SafeCalloc(num_items, sizeof(char *));
    for (int i = 0; i < num_items; ++i) {
        items[i] = (char *)SafeCalloc(item_length, sizeof(char));
    }
    return items;
}

static char **AllocateKeys(int num_items) {
    char **keys = (char **)SafeCalloc(num_items, sizeof(char *));
    for (int i = 0; i < num_items; ++i) {
        keys[i] = (char *)SafeCalloc(kKeyLengthMax, sizeof(char));
    }
    return keys;
}

static HookFunctionPointer *AllocateHooks(int num_items) {
    return (HookFunctionPointer *)SafeCalloc(num_items,
                                             sizeof(HookFunctionPointer));
}

static void FreeAll(int num_items, char **keys, HookFunctionPointer *hooks) {
    for (int i = 0; i < num_items; ++i) {
        free(items[i]);
        free(keys[i]);
    }
    free(items);
    items = NULL;
    free(keys);
    free(hooks);
}

#include "core/interactive/games/presolve/options/choices/choices.h"

#include <stddef.h>  // NULL
#include <stdio.h>   // sprintf, fprintf, stderr
#include <stdlib.h>  // atoi

#include "core/types/gamesman_types.h"
#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/match.h"
#include "core/misc.h"

static int option_index = -1;
// Hard-coded size based on the title definition in UpdateTitle.
static char title[43 + kSolverOptionNameLengthMax + kGameFormalNameLengthMax +
                  kSolverOptionChoiceNameLengthMax + 1];

static ReadOnlyString *AllocateItems(int num_items);
static char **AllocateKeys(int num_items);
static HookFunctionPointer *AllocateHooks(int num_items);

static void UpdateTitle(void);

static void MakeSelection(ReadOnlyString key);

static void FreeAll(int num_items, ReadOnlyString *items, char **keys,
                    HookFunctionPointer *hooks);

void InteractiveGameOptionChoices(ReadOnlyString key) {
    option_index = atoi(key);
    const GameVariant *variant = InteractiveMatchGetVariant();
    int num_items = variant->options[option_index].num_choices;
    ReadOnlyString *items = AllocateItems(num_items);
    char **keys = AllocateKeys(num_items);
    HookFunctionPointer *hooks = AllocateHooks(num_items);

    for (int i = 0; i < num_items; ++i) {
        items[i] = variant->options[option_index].choices[i];
        sprintf(keys[i], "%d", i);
        hooks[i] = &MakeSelection;
    }
    AutoMenu(title, num_items, (ConstantReadOnlyString *)items,
             (ConstantReadOnlyString *)keys, hooks, &UpdateTitle);
    FreeAll(num_items, items, keys, hooks);
}

static ReadOnlyString *AllocateItems(int num_items) {
    return (ReadOnlyString *)SafeCalloc(num_items, sizeof(ReadOnlyString));
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

static void MakeSelection(ReadOnlyString key) {
    int selection = atoi(key);
    int error = InteractiveMatchSetVariantOption(option_index, selection);
    if (error != 0) {
        fprintf(stderr, "MakeSelection: set variant option failed with code %d",
                error);
    }
}

static void UpdateTitle(void) {
    // Title:
    // "Changing option [<option_name>] for <game_name> (currently <selection>)"
    const Game *current_game = InteractiveMatchGetCurrentGame();
    const GameVariant *variant = InteractiveMatchGetVariant();
    int selection = variant->selections[option_index];
    sprintf(title, "Changing option [%s] for %s (currently %s)",
            variant->options[option_index].name, current_game->formal_name,
            variant->options[option_index].choices[selection]);
}

static void FreeAll(int num_items, ReadOnlyString *items, char **keys,
                    HookFunctionPointer *hooks) {
    free(items);
    for (int i = 0; i < num_items; ++i) {
        free(keys[i]);
    }
    free(keys);
    free(hooks);
}

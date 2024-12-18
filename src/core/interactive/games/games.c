#include "core/interactive/games/games.h"

#include <stdio.h>   // sprintf
#include <stdlib.h>  // free

#include "core/game_manager.h"  // GameManagerGetAllGames, GameManagerNumGames
#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/presolve.h"
#include "core/misc.h"                  // SafeMalloc
#include "core/types/gamesman_types.h"  // Game

static char **AllocateItems(int num_items) {
    char **items = (char **)SafeMalloc(num_items * sizeof(char *));
    for (int i = 0; i < num_items; ++i) {
        items[i] = (char *)SafeMalloc(kGameFormalNameLengthMax + 1);
    }
    return items;
}

static char **AllocateKeys(int num_items) {
    char **keys = (char **)SafeMalloc(num_items * sizeof(char *));
    for (int i = 0; i < num_items; ++i) {
        keys[i] = (char *)SafeMalloc(kKeyLengthMax);
    }
    return keys;
}

static HookFunctionPointer *AllocateHooks(int num_items) {
    return (HookFunctionPointer *)SafeMalloc(num_items *
                                             sizeof(HookFunctionPointer));
}

static void FreeAll(int num_items, char **items, char **keys,
                    HookFunctionPointer *hooks) {
    for (int i = 0; i < num_items; ++i) {
        free(items[i]);
        free(keys[i]);
    }
    free(items);
    free(keys);
    free(hooks);
}

int InteractiveGames(ReadOnlyString key) {
    (void)key;  // Unused.

    const Game *const *all_games = GameManagerGetAllGames();
    static ConstantReadOnlyString kTitle = "List of All Games";
    int num_items = GameManagerNumGames();
    char **items = AllocateItems(num_items);
    char **keys = AllocateKeys(num_items);
    HookFunctionPointer *hooks = AllocateHooks(num_items);
    for (int i = 0; i < num_items; ++i) {
        SafeStrncpy(items[i], all_games[i]->formal_name,
                    kGameFormalNameLengthMax + 1);
        items[i][kGameFormalNameLengthMax] = '\0';
        sprintf(keys[i], "%d", i);
        hooks[i] = &InteractivePresolve;
    }
    int ret = AutoMenu(kTitle, num_items, (ConstantReadOnlyString *)items,
                       (ConstantReadOnlyString *)keys,
                       (const HookFunctionPointer *)hooks, NULL);
    FreeAll(num_items, items, keys, hooks);

    return ret;
}

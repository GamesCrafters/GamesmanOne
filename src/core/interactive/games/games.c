#include "core/interactive/games/games.h"

#include <stdio.h>
#include <string.h>

#include "core/game_manager.h"
#include "core/gamesman_memory.h"
#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/presolve.h"
#include "core/types/base.h"
#include "core/types/game/game.h"

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
        keys[i] = (char *)SafeMalloc(kKeyLengthMax + 1);
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
        GamesmanFree(items[i]);
        GamesmanFree(keys[i]);
    }
    GamesmanFree(items);
    GamesmanFree(keys);
    GamesmanFree(hooks);
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
        strcpy(items[i], all_games[i]->formal_name);
        snprintf(keys[i], kKeyLengthMax + 1, "%d", i);
        hooks[i] = &InteractivePresolve;
    }
    int ret = AutoMenu(kTitle, num_items, (ConstantReadOnlyString *)items,
                       (ConstantReadOnlyString *)keys,
                       (const HookFunctionPointer *)hooks, NULL);
    FreeAll(num_items, items, keys, hooks);

    return ret;
}

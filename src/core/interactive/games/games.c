#include "core/interactive/games/games.h"

#include <stdio.h>   // sprintf
#include <stdlib.h>  // free

#include "core/gamesman_types.h"  // Game
#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/presolve.h"
#include "core/misc.h"           // SafeMalloc
#include "games/game_manager.h"  // GameManagerGetAllGames, GameManagerNumGames

static char **ListOfGamesAllocateItems(int num_items) {
    char **items = (char **)SafeMalloc(num_items * sizeof(char *));
    for (int i = 0; i < num_items; ++i) {
        items[i] = (char *)SafeMalloc(kGameFormalNameLengthMax + 1);
    }
    return items;
}

static char **ListOfGamesAllocateKeys(int num_items) {
    char **keys = (char **)SafeMalloc(num_items * sizeof(char *));
    for (int i = 0; i < num_items; ++i) {
        keys[i] = (char *)SafeMalloc(kKeyLengthMax);
    }
    return keys;
}

static HookFunctionPointer *ListOfGamesAllocateHooks(int num_items) {
    return (HookFunctionPointer *)SafeMalloc(num_items *
                                             sizeof(HookFunctionPointer));
}

static void ListOfGamesFreeAll(int num_items, char **items, char **keys,
                               HookFunctionPointer *hooks) {
    for (int i = 0; i < num_items; ++i) {
        free(items[i]);
        free(keys[i]);
    }
    free(items);
    free(keys);
    free(hooks);
}

void InteractiveGames(ReadOnlyString key) {
    (void)key;  // Unused.

    const Game *const *all_games = GameManagerGetAllGames();
    static ConstantReadOnlyString kTitle = "List of All Games";
    int num_items = GameManagerNumGames();
    char **items = ListOfGamesAllocateItems(num_items);
    char **keys = ListOfGamesAllocateKeys(num_items);
    HookFunctionPointer *hooks = ListOfGamesAllocateHooks(num_items);
    for (int i = 0; i < num_items; ++i) {
        SafeStrncpy(items[i], all_games[i]->formal_name, kGameFormalNameLengthMax + 1);
        items[i][kGameFormalNameLengthMax] = '\0';
        snprintf(keys[i], kKeyLengthMax, "%d", i);
        hooks[i] = &InteractivePresolve;
    }
    AutoMenu(kTitle, num_items, (ConstantReadOnlyString *)items,
             (ConstantReadOnlyString *)keys,
             (const HookFunctionPointer *)hooks);
    ListOfGamesFreeAll(num_items, items, keys, hooks);
}

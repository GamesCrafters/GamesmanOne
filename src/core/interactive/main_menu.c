#include "core/interactive/main_menu.h"

#include "core/interactive/automenu.h"
#include "core/interactive/credits/credits.h"
#include "core/interactive/games/games.h"
#include "core/interactive/help/help.h"

int InteractiveMainMenu(ReadOnlyString key) {
    (void)key;  // Unused.
    static ConstantReadOnlyString kTitle = "GAMESMAN Main Menu";
    static ConstantReadOnlyString items[] = {
        "List of Games",
        "GAMESMAN Help",
        "Credits",
    };
    static ConstantReadOnlyString keys[] = {"g", "h", "c"};
    static const HookFunctionPointer hooks[] = {
        &InteractiveGames,
        &InteractiveHelp,
        &InteractiveCredits,
    };
    int num_items = sizeof(items) / sizeof(items[0]);
    return AutoMenu(kTitle, num_items, items, keys, hooks, NULL);
}

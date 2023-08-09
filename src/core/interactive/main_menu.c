#include "core/interactive/main_menu.h"

#include "core/interactive/automenu.h"
#include "core/interactive/credits/credits.h"
#include "core/interactive/games/games.h"
#include "core/interactive/help/help.h"

void InteractiveMainMenu(const char *key) {
    (void)key;  // Unused.
    static const char *title = "Gamesman Main Menu";
    static const char *const items[] = {"List of Games", "GAMESMAN Help",
                                        "Credits"};
    static const char *const keys[] = {"g", "h", "c"};
    static const HookFunctionPointer hooks[] = {
        &InteractiveGames, &InteractiveHelp, &InteractiveCredits};
    AutoMenu(title, sizeof(items) / sizeof(items[0]), items, keys, hooks);
}

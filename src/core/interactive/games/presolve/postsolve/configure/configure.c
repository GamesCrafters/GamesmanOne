#include "core/interactive/games/presolve/postsolve/configure/configure.h"

#include <stdbool.h>
#include <stdio.h>

#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/match.h"

static void PrintPlayerConfiguration(void) {
    bool first_player_is_computer = InteractiveMatchPlayerIsComputer(0);
    bool second_player_is_computer = InteractiveMatchPlayerIsComputer(1);
    printf("First player is: %s\n",
           first_player_is_computer ? "computer" : "human");
    printf("Second player is: %s\n\n",
           second_player_is_computer ? "computer" : "human");
}

static void ToggleFirstPlayerType(ReadOnlyString key) {
    (void)key;  // Unused.
    InteractiveMatchTogglePlayerType(false);
    PrintPlayerConfiguration();
}

static void ToggleSecondPlayerType(ReadOnlyString key) {
    (void)key;  // Unused.
    InteractiveMatchTogglePlayerType(true);
    PrintPlayerConfiguration();
}

void InteractivePostSolveConfigure(ReadOnlyString key) {
    (void)key;  // Unused.
    PrintPlayerConfiguration();
    static ConstantReadOnlyString kTitle = "Configuration Menu";
    static ConstantReadOnlyString items[] = {
        "Toggle player type of the first player",
        "Toggle player type of the second player",
    };
    static ConstantReadOnlyString keys[] = {"1", "2"};
    static const HookFunctionPointer hooks[] = {
        &ToggleFirstPlayerType,
        &ToggleSecondPlayerType,
    };
    int num_items = sizeof(items) / sizeof(items[0]);
    AutoMenu(kTitle, num_items, items, keys, hooks, NULL);
}

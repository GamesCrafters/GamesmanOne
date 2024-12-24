#include "core/interactive/games/presolve/postsolve/postsolve.h"

#include <stdio.h>

#include "core/constants.h"
#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/match.h"
#include "core/interactive/games/presolve/postsolve/analyze/analyze.h"
#include "core/interactive/games/presolve/postsolve/configure/configure.h"
#include "core/interactive/games/presolve/postsolve/help/game_help.h"
#include "core/interactive/games/presolve/postsolve/play/play.h"
#include "core/types/gamesman_types.h"

static const char title_format[] =
    "Play (Post-Solved) Menu for %s (variant %d)";
static char title[sizeof(title_format) + kGameFormalNameLengthMax +
                  kInt32Base10StringLengthMax];

static void UpdateVariantId(void) {
    const Game *current_game = InteractiveMatchGetCurrentGame();
    int variant_index = InteractiveMatchGetVariantIndex();
    sprintf(title, title_format, current_game->formal_name, variant_index);
}

int InteractivePostSolve(ReadOnlyString key) {
    (void)key;  // Unused.
    static ConstantReadOnlyString items[] = {
        "Play new game",
        "Configure play options",
        "Analyze the game",
        "Help",
    };
    static ConstantReadOnlyString keys[] = {"p", "c", "a", "h"};
    static const HookFunctionPointer hooks[] = {
        &InteractivePlay,
        &InteractivePostSolveConfigure,
        &InteractiveAnalyze,
        &InteractiveGameHelp,
    };
    int num_items = sizeof(items) / sizeof(items[0]);

    return AutoMenu(title, num_items, items, keys, hooks, &UpdateVariantId);
}

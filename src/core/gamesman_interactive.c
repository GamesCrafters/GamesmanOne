#include "core/gamesman_interactive.h"

#include <stddef.h>  // NULL

#include "core/interactive/main_menu.h"

int GamesmanInteractiveMain(void) {
    printf("This is a sample welcome message.\n");
    printf("--- press <return> to continue ---");
    getchar();
    InteractiveMainMenu(NULL);
    return 0;
}

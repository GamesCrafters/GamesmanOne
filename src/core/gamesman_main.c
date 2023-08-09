// Replace the old main.c file with this when done.

#include <stdio.h>

#include "core/gamesman_headless.h"
#include "core/gamesman_interactive.h"

int main(int argc, char **argv) { 
    if (argc == 1) {
        return GamesmanInteractiveMain();
    }
    return GamesmanHeadlessMain(argc, argv);
}

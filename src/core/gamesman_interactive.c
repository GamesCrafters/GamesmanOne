#include "core/gamesman_interactive.h"

#include <stddef.h>  // NULL
#include <stdio.h>   // printf
#include <unistd.h>  // usleep

#include "core/interactive/main_menu.h"

static const char *kOpeningCredits =
    "\n"
    "  ____               http://gamescrafters.berkeley.edu  : A finite,  two-person\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _ __   : complete  information\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` | '_ \\  : game generator.  More\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| | | | | : information?  Contact\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|_| |_| : ddgarcia@berkeley.edu\n"
    "...............................................................................\n"
    "\n"
    "Welcome to GAMESMAN, version 2023.08.14. Originally       (G)ame-independent\n"
    "written by Dan Garcia, it has undergone a series of       (A)utomatic       \n"
    "exhancements from 2001-present by GamesCrafters, the      (M)ove-tree       \n"
    "UC Berkeley Undergraduate Game Theory Research Group.     (E)xhaustive      \n"
    "                                                          (S)earch,         \n"
    "This program will determine the value of your game,       (M)anipulation    \n"
    "perform analysis, & provide an interface to play it.      (A)nd             \n"
    "                                                          (N)avigation      \n";

void PrintOpeningCredits(void) {
    int i = 0;
    while (kOpeningCredits[i] != '\0') {
        printf("%c", kOpeningCredits[i++]);
        fflush(stdout);
        usleep(800);
    }
}

int GamesmanInteractiveMain(void) {
    PrintOpeningCredits();
    printf("--- press <return> to continue ---");
    getchar();
    InteractiveMainMenu(NULL);
    return 0;
}

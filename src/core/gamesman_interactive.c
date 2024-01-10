/**
 * @file gamesman_interactive.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of GAMESMAN interactive mode.
 * @version 1.1.0
 * @date 2023-10-21
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/gamesman_interactive.h"

#include <stddef.h>  // NULL
#include <stdio.h>   // printf
#include <stdlib.h>  // free
#include <string.h>  // strlen
#include <unistd.h>  // usleep

#include "core/constants.h"
#include "core/types/gamesman_types.h"
#include "core/interactive/main_menu.h"
#include "core/misc.h"

static ConstantReadOnlyString kOpeningCreditsFormat =
    "\n"
    "  ____               http://gamescrafters.berkeley.edu  : A finite,  two-person\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _ __   : complete  information\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` | '_ \\  : game generator.  More\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| | | | | : information?  Contact\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|_| |_| : ddgarcia@berkeley.edu\n"
    "...............................................................................\n"
    "\n"
    "Welcome to GAMESMAN, version %s. Originally       (G)ame-independent\n"
    "written by Dan Garcia, it has undergone a series of       (A)utomatic       \n"
    "exhancements from 2001-present by GamesCrafters, the      (M)ove-tree       \n"
    "UC Berkeley Undergraduate Game Theory Research Group.     (E)xhaustive      \n"
    "                                                          (S)earch,         \n"
    "This program will determine the value of your game,       (M)anipulation    \n"
    "perform analysis, & provide an interface to play it.      (A)nd             \n"
    "                                                          (N)avigation      \n";

void PrintOpeningCredits(void) {
    size_t length = strlen(kOpeningCreditsFormat) + strlen(kGamesmanDate);
    char *opening_credits = (char *)SafeCalloc(length, sizeof(char));
    sprintf(opening_credits, kOpeningCreditsFormat, kGamesmanDate);

    int i = 0;
    while (opening_credits[i] != '\0') {
        printf("%c", opening_credits[i++]);
        fflush(stdout);
        usleep(800);
    }

    free(opening_credits);
}

int GamesmanInteractiveMain(void) {
    PrintOpeningCredits();
    printf("--- press <return> to continue ---");
    getchar();
    InteractiveMainMenu(NULL);
    return 0;
}

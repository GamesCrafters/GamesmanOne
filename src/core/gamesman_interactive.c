/**
 * @file gamesman_interactive.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of GAMESMAN interactive mode.
 * @version 1.1.2
 * @date 2025-04-26
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

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "core/gamesman_memory.h"
#include "core/interactive/main_menu.h"
#include "core/opening_credits.h"
#include "core/types/base.h"
#include "core/types/gamesman_status.h"

#ifdef GAMESMAN_ENABLE_ANIMATION
#include <unistd.h>
#endif  // GAMESMAN_ENABLE_ANIMATION

// clang-format off
static ConstantReadOnlyString kOpeningCreditsFormat =
    "\n%s"RESET
#ifndef NDEBUG
    "...............................................................................\n"
    "................................. DEBUG BUILD .................................\n"
#endif
    "..........................%s.............................\n"
    "\n"
    "Welcome to GAMESMAN, version "THEME"%s"RESET". Originally       (G)ame-independent\n"
    "written by Dan Garcia, it has undergone a series of       (A)utomatic       \n"
    "exhancements from 2001-present by GamesCrafters, the      (M)ove-tree       \n"
    "UC Berkeley Computational Game Theory Research and        (E)xhaustive      \n"
    "Development Group.                                        (S)earch,         \n"
    "                                                          (M)anipulation    \n"
    "This program will determine the value of your game,       (A)nd             \n"
    "perform analysis, & provide an interface to play it.      (N)avigation      \n"
    "\n";
// clang-format on

static const char *const kContinuePrompt = "--- press <return> to continue ---";

static const int kOpeningCreditsMessageSize = 24;
#ifndef USE_MPI
static ConstantReadOnlyString kOpeningCreditsNoMessage =
    "........................";
#else   // USE_MPI defined.
static ConstantReadOnlyString kOpeningCreditsMpiMessage =
    "      MPI Enabled       ";
#endif  // USE_MPI

static void AnimationUpdate(char *opening_credits, size_t capacity, int frame) {
#ifndef USE_MPI
    snprintf(opening_credits, capacity, kOpeningCreditsFormat,
             kHeaderAnimation[frame], kOpeningCreditsNoMessage, GM_DATE);
#else   // USE_MPI defined.
    snprintf(opening_credits, capacity, kOpeningCreditsFormat,
             kHeaderAnimation[frame], kOpeningCreditsMpiMessage, GM_DATE);
#endif  // USE_MPI
}

#ifdef GAMESMAN_ENABLE_ANIMATION
static void EraseLastBlockExact(const char *s) {
    if (!s) return;

    size_t len = strlen(s);
    if (len == 0) return;

    // Count newlines
    size_t nl = 0;
    for (const char *p = s; *p; ++p) {
        if (*p == '\n') ++nl;
    }
    int none_nl_terminated_last_line = (s[len - 1] != '\n');

    // Number of lines that actually contained text
    size_t printed_lines = nl + none_nl_terminated_last_line;

    // Move to the block's first line, column 1
    if (none_nl_terminated_last_line) {
        // Cursor is at end of the last line of the block
        putchar('\r');  // column 1
        if (printed_lines > 1) {
            printf("\x1b[%zuA", printed_lines - 1);  // up to first line
        }
    } else {
        // Cursor is currently on the line AFTER the block
        printf("\x1b[%zuA", printed_lines);  // up to first line of block
        putchar('\r');
    }

    // Clear each printed line, moving downward
    for (size_t i = 0; i < printed_lines; ++i) {
        printf("\x1b[2K");                             // erase entire line
        if (i + 1 < printed_lines) printf("\x1b[1B");  // down 1
    }

    // Return cursor to the top of the cleared block (col 1)
    if (printed_lines > 1) printf("\x1b[%zuA", printed_lines - 1);
    putchar('\r');
    fflush(stdout);
}
#endif  // GAMESMAN_ENABLE_ANIMATION

static void PrintOpeningCredits(void) {
    const size_t length = strlen(kHeaderAnimation[0]) +
                          strlen(kOpeningCreditsFormat) + strlen(GM_DATE) +
                          kOpeningCreditsMessageSize;
    char *buf = (char *)SafeCalloc(length + 1, sizeof(char));

    const int nframes = sizeof(kHeaderAnimation) / sizeof(kHeaderAnimation[0]);
#ifdef GAMESMAN_ENABLE_ANIMATION
    char *prev = NULL;
    for (int i = 0; i < nframes; ++i) {
        EraseLastBlockExact(prev);
        AnimationUpdate(buf, length, i);
        prev = buf;
        printf("%s", buf);
        fflush(stdout);
        usleep(8000);
    }
#else   // No animation
    AnimationUpdate(buf, length, nframes - 1);
    printf("%s", buf);
    fflush(stdout);
#endif  // GAMESMAN_ENABLE_ANIMATION
    GamesmanFree(buf);
}

#ifdef GAMESMAN_ENABLE_ANIMATION
static void AnimateText(const char *str, unsigned int us) {
    while (*str) {
        putchar(*(str++));
        fflush(stdout);
        usleep(us);
    }
}
#endif  // GAMESMAN_ENABLE_ANIMATION

static void PromptForContinue(void) {
#ifdef GAMESMAN_ENABLE_ANIMATION
    AnimateText(kContinuePrompt, 5000);
#else   // No animation
    printf("%s", kContinuePrompt);
#endif  // GAMESMAN_ENABLE_ANIMATION
    getchar();
}

int GamesmanInteractiveMain(void) {
    PrintOpeningCredits();
    PromptForContinue();
    InteractiveMainMenu(NULL);

    return kSuccess;
}

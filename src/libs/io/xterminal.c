/**
 * @file xterminal.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Terminal I/O helper functions implementation.
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

#include "libs/io/xterminal.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void PrintfAndFlush(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    fflush(stdout);
    va_end(args);
}

char *PromptForInput(const char *prompt, char *buf, int max_length) {
    printf("%s\n=> ", prompt);
    if (fgets(buf, max_length + 1, stdin) == NULL) {
        return NULL;
    }

    // Clear the stdin buffer if the input was too long
    if (strchr(buf, '\n') == NULL) {
        int ch = getchar();
        while (ch != '\n' && ch != EOF) {
            ch = getchar();
        }
    }

    // Remove the trailing newline character, if it exists.
    // Algorithm by Tim Čas,
    // https://stackoverflow.com/a/28462221.
    buf[strcspn(buf, "\r\n")] = '\0';

    return buf;
}

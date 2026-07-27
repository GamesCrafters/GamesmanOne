/**
 * @file misc.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of miscellaneous utility functions.
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

#include "core/misc.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/types/gamesman_error.h"

void GamesmanExit(void) {
    printf("Thanks for using GAMESMAN!\n");
    exit(kNoError);  // NOLINT(concurrency-mt-unsafe)
}

void NotReached(const char *message) {
    if (message) {
        fprintf(
            stderr,
            "(FATAL) You entered a branch that is marked as NotReached. The "
            "error message was %s\n",
            message);
    } else {
        fprintf(stderr,
                "(FATAL) You entered a branch that is marked as NotReached.\n");
    }
    fflush(stderr);
    _exit(kNotReachedError);
}

static void AppendIfPositive(char *buf, size_t buf_size, int val,
                             const char *label) {
    if (val <= 0) return;
    size_t len = strlen(buf);
    if (len < buf_size) {
        snprintf(buf + len, buf_size - len, "%d %s", val, label);
    }
}

char *SecondsToFormattedTimeString(double seconds_d, char *buf,
                                   size_t buf_size) {
    if (!buf || buf_size == 0) {
        return NULL;
    }
    if (seconds_d < 0.0) {
        snprintf(buf, buf_size, "NEGATIVE TIME ERROR");
        return buf;
    }
    if (seconds_d > (double)INT64_MAX) {
        snprintf(buf, buf_size, "INFINITE");
        return buf;
    }

    int years = 0, months = 0, days = 0, hours = 0, minutes = 0, seconds = 0;
    int64_t remainder = (int64_t)seconds_d;
    seconds = (int)(remainder % 60);
    remainder /= 60;
    minutes = (int)(remainder % 60);
    remainder /= 60;
    hours = (int)(remainder % 24);
    remainder /= 24;
    days = (int)(remainder % 30);
    remainder /= 30;
    months = (int)(remainder % 12);
    remainder /= 12;
    years = (int)(remainder > 9999 ? -1 : remainder);
    if (years < 0) {
        snprintf(buf, buf_size, "INFINITE");
    } else {
        buf[0] = '\0';
        AppendIfPositive(buf, buf_size, years, "y ");
        AppendIfPositive(buf, buf_size, months, "m ");
        AppendIfPositive(buf, buf_size, days, "d ");
        AppendIfPositive(buf, buf_size, hours, "h ");
        AppendIfPositive(buf, buf_size, minutes, "m ");
        size_t len = strlen(buf);
        if (len < buf_size) {
            snprintf(buf + len, buf_size - len, "%d s", seconds);
        }
    }

    return buf;
}

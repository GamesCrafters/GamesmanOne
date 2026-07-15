/**
 * @file misc.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of miscellaneous utility functions.
 * @version 2.0.0
 * @date 2025-03-18
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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "core/types/base.h"
#ifdef USE_MPI
#include <mpi.h>
#endif  // USE_MPI

#include "core/types/gamesman_error.h"

void GamesmanExit(void) {
    printf("Thanks for using GAMESMAN!\n");
    exit(kNoError);  // NOLINT(concurrency-mt-unsafe)
}

void NotReached(ReadOnlyString message) {
    fprintf(stderr,
            "(FATAL) You entered a branch that is marked as NotReached. The "
            "error message was %s\n",
            message);
    fflush(stderr);
    _exit(kNotReachedError);
}

char *SafeStrncpy(char *dest, const char *src, size_t n) {
    char *ret = strncpy(dest, src, n);
    dest[n - 1] = '\0';
    return ret;
}

void PrintfAndFlush(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    fflush(stdout);
    va_end(args);
}

char *PromptForInput(ReadOnlyString prompt, char *buf, int length_max) {
    printf("%s\n=> ", prompt);
    if (fgets(buf, length_max + 1, stdin) == NULL) return NULL;

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

char *GetTimeStampString(void) {
    time_t rawtime = time(NULL);
    static char time_str[26];  // 26 bytes as requested by ctime_r.
    ctime_r(&rawtime, time_str);
    time_str[strlen(time_str) - 1] = '\0';  // Get rid of the trailing '\n'.

    return time_str;
}

static void AppendIfPositive(char *buf, int val, ReadOnlyString label) {
    if (val <= 0) return;
    sprintf(buf + strlen(buf), "%d %s", val, label);
}

char *SecondsToFormattedTimeString(double _seconds) {
    // Format is the following or "INFINITE" if yyyy is greater than 9999.
    static const char format[] = "[yyyy y mm m dd d hh h mm m ]ss s";
    static char buf[sizeof(format)];
    if (_seconds < 0.0) {
        sprintf(buf, "NEGATIVE TIME ERROR");
        return buf;
    }

    int years = 0, months = 0, days = 0, hours = 0, minutes = 0, seconds;
    int64_t remainder = (int64_t)_seconds;
    seconds = (int)(remainder % 60);
    remainder /= 60;
    minutes = (int)(remainder % 60);
    remainder /= 60;
    hours = (int)(remainder % 24);
    remainder /= 24;
    days = (int)(remainder % 30);
    remainder /= 30;
    months = (int)(remainder %= 12);
    remainder /= 12;
    years = (int)(remainder > 9999 ? -1 : remainder);
    if (years < 0) {
        sprintf(buf, "INFINITE");
    } else {
        buf[0] = '\0';
        AppendIfPositive(buf, years, "y ");
        AppendIfPositive(buf, months, "m ");
        AppendIfPositive(buf, days, "d ");
        AppendIfPositive(buf, hours, "h ");
        AppendIfPositive(buf, minutes, "m ");
        sprintf(buf + strlen(buf), "%d %s", seconds, "s");
    }

    return buf;
}

#ifdef USE_MPI

void SafeMpiInitThread(int *argc, char ***argv, int required, int *provided) {
    int error = MPI_Init_thread(argc, argv, required, provided);
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiInitThread: failed with code %d\n", error);
        fflush(stderr);
        _exit(kMpiError);
    }
}

void SafeMpiInit(int *argc, char ***argv) {
    int error = MPI_Init(argc, argv);
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiInit: failed with code %d\n", error);
        exit(kMpiError);
    }
}

void SafeMpiFinalize(void) {
    int error = MPI_Finalize();
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiFinalize: failed with code %d\n", error);
        fflush(stderr);
        _exit(kMpiError);
    }
}

int SafeMpiCommSize(MPI_Comm comm) {
    int ret;
    int error = MPI_Comm_size(comm, &ret);
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiCommSize: failed with code %d\n", error);
        fflush(stderr);
        _exit(kMpiError);
    }

    return ret;
}

int SafeMpiCommRank(MPI_Comm comm) {
    int ret;
    int error = MPI_Comm_rank(comm, &ret);
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiCommRank: failed with code %d\n", error);
        fflush(stderr);
        _exit(kMpiError);
    }

    return ret;
}

void SafeMpiSend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                 MPI_Comm comm) {
    int error = MPI_Send(buf, count, datatype, dest, tag, comm);
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiSend: failed with code %d\n", error);
        fflush(stderr);
        _exit(kMpiError);
    }
}

void SafeMpiRecv(void *buf, int count, MPI_Datatype datatype, int source,
                 int tag, MPI_Comm comm, MPI_Status *status) {
    int error = MPI_Recv(buf, count, datatype, source, tag, comm, status);
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiRecv: failed with code %d\n", error);
        fflush(stderr);
        _exit(kMpiError);
    }
}
#endif  // USE_MPI

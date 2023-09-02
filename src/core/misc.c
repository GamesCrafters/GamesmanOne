/**
 * @file misc.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of miscellaneous utility functions.
 * @version 1.0
 * @date 2023-08-19
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

#include <assert.h>    // assert
#include <errno.h>     // errno
#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // size_t
#include <stdint.h>    // int64_t, INT64_MAX
#include <stdio.h>     // fprintf, stderr, FILE
#include <stdlib.h>    // exit, EXIT_SUCCESS, EXIT_FAILURE, malloc, calloc, free
#include <string.h>    // strlen, strncpy
#include <sys/stat.h>  // mkdir, struct stat
#include <sys/types.h>  // mode_t

#include "core/gamesman_types.h"

void GamesmanExit(void) {
    printf("Thanks for using GAMESMAN!\n");
    exit(EXIT_SUCCESS);
}

void NotReached(ReadOnlyString message) {
    fprintf(stderr,
            "(FATAL) You entered a branch that is marked as NotReached. The "
            "error message was %s\n",
            message);
    exit(EXIT_FAILURE);
}

void *SafeMalloc(size_t size) {
    void *ret = malloc(size);
    if (ret == NULL) {
        fprintf(stderr,
                "SafeMalloc: failed to allocate %zd bytes. This ususally "
                "indicates a bug.\n",
                size);
        exit(EXIT_FAILURE);
    }
    return ret;
}

void *SafeCalloc(size_t n, size_t size) {
    void *ret = calloc(n, size);
    if (ret == NULL) {
        fprintf(stderr,
                "SafeCalloc: failed to allocate %zd elements each of %zd "
                "bytes. This ususally "
                "indicates a bug.\n",
                n, size);
        exit(EXIT_FAILURE);
    }
    return ret;
}

FILE *SafeFopen(const char *filename, const char *modes) {
    FILE *f = fopen(filename, modes);
    if (f == NULL) perror("fopen");
    return f;
}

int SafeFclose(FILE *stream) {
    int error = fclose(stream);
    if (error != 0) perror("fclose");
    return error;
}

int SafeFwrite(void *ptr, size_t size, size_t n, FILE *stream) {
    if (fwrite(ptr, size, n, stream) != n) {
        perror("fwrite");
        return errno;
    }
    return 0;
}

/**
 * @brief Makes a directory at the given path or does nothing if the directory
 * already exists.
 *
 * @return 0 on success. On error, -1 is returned and errno is set to indicate
 * the error.
 *
 * @author Jonathon Reinhart
 * @link
 * https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
 */
static int MaybeMkdir(ReadOnlyString path, mode_t mode) {
    errno = 0;

    // Try to make the directory
    if (mkdir(path, mode) == 0) return 0;

    // If it fails for any reason but EEXIST, fail
    if (errno != EEXIST) return -1;

    // Check if the existing path is a directory
    struct stat st;
    if (stat(path, &st) != 0) return -1;

    // If not, fail with ENOTDIR
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }

    errno = 0;
    return 0;
}

int MkdirRecursive(ReadOnlyString path) {
    // Fail if path is NULL.
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }

    int result = -1;
    errno = 0;

    // Copy string so it's mutable
    size_t path_length = strlen(path);
    char *path_copy = (char *)malloc((path_length + 1) * sizeof(char));
    if (path_copy == NULL) {
        errno = ENOMEM;
        goto _bailout;
    }
    strncpy(path_copy, path, path_length + 1);

    for (size_t i = 0; i < path_length; ++i) {
        if (path_copy[i] == '/') {
            path_copy[i] = '\0';  // Temporarily truncate
            if (MaybeMkdir(path_copy, 0777) != 0) goto _bailout;
            path_copy[i] = '/';
        }
    }
    if (MaybeMkdir(path_copy, 0777) != 0) goto _bailout;
    result = 0;  // Success

_bailout:
    free(path_copy);
    return result;
}

bool IsPrime(int64_t n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int64_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
    }
    return true;
}

int64_t PrevPrime(int64_t n) {
    if (n < 2) return 2;
    while (!IsPrime(n)) {
        --n;
    }
    return n;
}

int64_t NextPrime(int64_t n) {
    while (!IsPrime(n)) {
        ++n;
    }
    return n;
}

int64_t SafeAddNonNegativeInt64(int64_t a, int64_t b) {
    if (a < 0 || b < 0 || a > INT64_MAX - b) return -1;
    return a + b;
}

int64_t SafeMultiplyNonNegativeInt64(int64_t a, int64_t b) {
    if (a < 0 || b < 0 || a > INT64_MAX / b) return -1;
    return a * b;
}

static int64_t NChooseRFormula(int n, int r) {
    assert(n >= 0 && r >= 0 && n >= r);

    // nCr(n, r) == nCr(n, n-r). This step can further reduce the largest
    // intermediate value.
    if (r > n - r) r = n - r;
    int64_t result = 1;
    for (int64_t i = 1; i <= r; ++i) {
        result = SafeMultiplyNonNegativeInt64(result, n - r + i);
        if (result < 0) return -1;
        result /= i;
    }
    return result;
}

#define CACHE_ROWS 100
#define CACHE_COLS 100
// Assumes CHOOSE has been zero initialized.
static void MakeTriangle(int64_t choose[][CACHE_COLS]) {
    for (int i = 0; i < CACHE_ROWS; ++i) {
        choose[i][0] = 1;
        int j_max = (i < CACHE_COLS - 1) ? i : CACHE_COLS - 1;
        for (int j = 1; j <= j_max; ++j) {
            choose[i][j] =
                SafeAddNonNegativeInt64(choose[i - 1][j - 1], choose[i - 1][j]);
        }
    }
}

int64_t NChooseR(int n, int r) {
    // Initialize cache.
    static bool choose_initialized = false;
    static int64_t choose[CACHE_ROWS][CACHE_COLS] = {0};

    if (!choose_initialized) {
        MakeTriangle(choose);
        choose_initialized = true;
    }

    if (n < 0 || r < 0) return -1;  // Negative inputs not supported.
    if (n < r) return 0;  // Make sure n >= r >= 0 in the following steps.
    if (n < CACHE_ROWS && r < CACHE_COLS) return choose[n][r];  // Cache hit.
    return NChooseRFormula(n, r);  // Cache miss. Calculate from formula.
}
#undef CACHE_ROWS
#undef CACHE_COLS

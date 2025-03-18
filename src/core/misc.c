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

#include <assert.h>     // assert
#include <errno.h>      // errno
#include <fcntl.h>      // open
#include <inttypes.h>   // PRId64, PRIu64
#include <lzma.h>       // lzma_physmem
#include <stdarg.h>     // va_list, va_start, va_end
#include <stdbool.h>    // bool, true, false
#include <stddef.h>     // size_t
#include <stdint.h>     // int64_t, uint32_t, uint64_t, INT64_MAX, uint8_t
#include <stdio.h>      // fgets, fprintf, stderr, FILE, rename
#include <stdlib.h>     // exit, malloc, calloc, aligned_alloc, free
#include <string.h>     // strcspn, strlen, strncpy, memset
#include <sys/stat.h>   // mkdir, struct stat
#include <sys/types.h>  // mode_t
#include <time.h>       // clock_t, CLOCKS_PER_SEC
#include <unistd.h>     // close, _exit
#include <zlib.h>  // gzFile, gzopen, gzdopen, gzread, gzwrite, Z_NULL, Z_OK
#ifdef USE_MPI
#include <mpi.h>
#endif  // USE_MPI

#include "core/types/gamesman_types.h"
#include "libs/mgz/gz64.h"

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

intptr_t GetPhysicalMemory(void) { return (intptr_t)lzma_physmem(); }

void *SafeMalloc(size_t size) {
    void *ret = malloc(size);
    if (ret == NULL) {
        fprintf(stderr,
                "SafeMalloc: failed to allocate %zd bytes. This ususally "
                "indicates a bug.\n",
                size);
        fflush(stderr);
        _exit(kMallocFailureError);
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
        fflush(stderr);
        _exit(kMallocFailureError);
    }
    return ret;
}

static bool IsValidAlignment(size_t alignment) {
    // Must be a multiple of pointer size.
    if (alignment % sizeof(void *)) return false;

    // Must be a power of 2.
    if (alignment & (alignment - 1)) return false;

    return true;
}

void *AlignedCalloc(size_t alignment, size_t n, size_t size) {
    if (!IsValidAlignment(alignment)) return NULL;

    size_t desired = n * size;
    size_t required = (desired + alignment - 1) / alignment * alignment;
    void *ret = aligned_alloc(alignment, required);
    if (ret == NULL) return ret;

    memset(ret, 0, required);

    return ret;
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

void *GenericPointerAdd(const void *p, int64_t offset) {
    return (void *)((uint8_t *)p + offset);
}

double ClockToSeconds(clock_t n) { return (double)n / CLOCKS_PER_SEC; }

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

FILE *GuardedFopen(const char *filename, const char *modes) {
    FILE *f = fopen(filename, modes);
    if (f == NULL) perror("fopen");
    return f;
}

FILE *GuardedFreopen(const char *filename, const char *modes, FILE *stream) {
    FILE *ret = freopen(filename, modes, stream);
    if (ret == NULL) perror("freopen");
    return ret;
}

int GuardedFclose(FILE *stream) {
    int error = fclose(stream);
    if (error != 0) perror("fclose");

    return error;
}

int BailOutFclose(FILE *stream, int error) {
    int fclose_error = fclose(stream);
    if (fclose_error != 0) perror("fclose");

    return error;
}

int GuardedFseek(FILE *stream, long off, int whence) {
    int error = fseek(stream, off, whence);
    if (error != 0) perror("fseek");

    return error;
}

int GuardedFread(void *ptr, size_t size, size_t n, FILE *stream, bool eof_ok) {
    size_t items_read = fread(ptr, size, n, stream);
    if (items_read == n) return 0;
    if (feof(stream)) {
        if (eof_ok) return 0;

        fprintf(
            stderr,
            "GuardedFread: end-of-file reached before reading %zd items, only "
            "%zd items were actually read\n",
            n, items_read);
        return 2;
    } else if (ferror(stream)) {
        fprintf(stderr, "GuardedFread: fread() error\n");
        return 3;
    }
    NotReached("GuardedFread: unknown error occurred during fread()");
    return 4;
}

int GuardedFwrite(const void *ptr, size_t size, size_t n, FILE *stream) {
    if (fwrite(ptr, size, n, stream) != n) {
        perror("fwrite");
        return errno;
    }
    return 0;
}

int GuardedOpen(const char *filename, int flags) {
    int fd = open(filename, flags);
    if (fd == -1) perror("open");

    return fd;
}

int GuardedClose(int fd) {
    int error = close(fd);
    if (error == -1) perror("close");

    return error;
}

int GuardedRename(const char *oldpath, const char *newpath) {
    int error = rename(oldpath, newpath);
    if (error == -1) perror("rename");

    return error;
}

int GuardedRemove(const char *pathname) {
    int error = remove(pathname);
    if (error == -1) perror("remove");

    return error;
}

int BailOutClose(int fd, int error) {
    int new_error = close(fd);
    if (new_error == -1) perror("close");

    return error;
}

int GuardedLseek(int fd, off_t offset, int whence) {
    off_t sought = lseek(fd, offset, whence);
    if (sought != offset) {
        perror("lseek");
        return -1;
    }

    return 0;
}

gzFile GuardedGzopen(const char *path, const char *mode) {
    gzFile file = gzopen(path, mode);
    if (file == Z_NULL) perror("gzopen");

    return file;
}

gzFile GuardedGzdopen(int fd, const char *mode) {
    gzFile file = gzdopen(fd, mode);
    if (file == Z_NULL) perror("gzdopen");

    return file;
}

int GuardedGzclose(gzFile file) {
    int error = gzclose(file);
    if (error != Z_OK) perror("gzclose");

    return error;
}

int BailOutGzclose(gzFile file, int error) {
    int new_error = gzclose(file);
    if (new_error != Z_OK) perror("gzclose");

    return error;
}

int GuardedGzseek(gzFile file, off_t off, int whence) {
    off_t sought = gzseek(file, off, whence);
    if (sought != off) {
        perror("gzseek");
        return -1;
    }

    return 0;
}

int GuardedGzread(gzFile file, voidp buf, unsigned int length, bool eof_ok) {
    int bytes_read = gzread(file, buf, length);
    if ((unsigned int)bytes_read == length) return 0;

    int error;
    if (gzeof(file)) {
        if (eof_ok) return 0;
        fprintf(
            stderr,
            "GuardedGzread: end-of-file reached before reading %d bytes, only "
            "%d bytes were actually read\n",
            (int)length, bytes_read);
        return 2;
    } else if (gzerror(file, &error)) {
        fprintf(stderr, "GuardedGzread: gzread() error code %d\n", error);
        return 3;
    }
    NotReached("GuardedGzread: unknown error occurred during gzread()");
    return 4;
}

int GuardedGz64Read(gzFile file, voidp buf, uint64_t length, bool eof_ok) {
    int64_t bytes_read = gz64_read(file, buf, length);
    if ((uint64_t)bytes_read == length) return 0;

    int error;
    if (gzeof(file)) {
        if (eof_ok) return 0;
        fprintf(stderr,
                "GuardedGz64Read: end-of-file reached before reading %" PRIu64
                " bytes, only %" PRId64 " bytes were actually read\n",
                length, bytes_read);
        return 2;
    } else if (gzerror(file, &error)) {
        fprintf(stderr, "GuardedGz64Read: gzread() error code %d\n", error);
        return 3;
    }
    NotReached("GuardedGz64Read: unknown error occurred during gzread()");
    return 4;
}

int GuardedGzwrite(gzFile file, voidpc buf, unsigned int len) {
    int bytes_written = gzwrite(file, buf, len);
    if ((unsigned int)bytes_written < len) {
        int error;
        gzerror(file, &error);
        fprintf(stderr, "GuardedGzwrite: failed with code %d\n", error);
        return error;
    }
    return 0;
}

bool FileExists(ReadOnlyString filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return true;
    }

    return false;
}

/**
 * @brief Makes a directory at the given path or does nothing if the directory
 * already exists.
 *
 * @return 0 on success. On error, kFileSystemError is returned and errno is set
 * to indicate the error.
 *
 * @author Jonathon Reinhart
 * @link
 * https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
 */
static int MaybeMkdir(ReadOnlyString path, mode_t mode) {
    errno = 0;

    // Try to make the directory
    if (mkdir(path, mode) == 0) return kNoError;

    // If it fails for any reason but EEXIST, fail
    if (errno != EEXIST) return kFileSystemError;

    // Check if the existing path is a directory
    struct stat st;
    if (stat(path, &st) != 0) return kFileSystemError;

    // If not, fail with ENOTDIR
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return kFileSystemError;
    }

    errno = 0;
    return kNoError;
}

int MkdirRecursive(ReadOnlyString path) {
    // Fail if path is NULL.
    if (path == NULL) {
        errno = EINVAL;
        return kIllegalArgumentError;
    }

    int ret = kFileSystemError;
    errno = 0;

    // Copy string so it's mutable
    size_t path_length = strlen(path);
    char *path_copy = (char *)malloc((path_length + 1) * sizeof(char));
    if (path_copy == NULL) {
        ret = kMallocFailureError;
        errno = ENOMEM;
        goto _bailout;
    }
    SafeStrncpy(path_copy, path, path_length + 1);

    for (size_t i = 0; i < path_length; ++i) {
        if (path_copy[i] == '/') {
            path_copy[i] = '\0';  // Temporarily truncate
            if (MaybeMkdir(path_copy, 0777) != 0) goto _bailout;
            path_copy[i] = '/';
        }
    }
    if (MaybeMkdir(path_copy, 0777) != 0) goto _bailout;
    ret = kNoError;  // Success

_bailout:
    free(path_copy);
    return ret;
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

int64_t RoundUpDivide(int64_t n, int64_t d) { return (n + d - 1) / d; }

#if !defined(__GNUC__) && !defined(__clang__) && !defined(_MSC_VER)
static int FallbackPopcount32(uint32_t x) {
    int c = 0;
    while (x) {
        x &= (x - 1);
        c++;
    }
    return c;
}

static int FallbackPopcount64(uint64_t x) {
    int c = 0;
    while (x) {
        x &= (x - 1);
        c++;
    }
    return c;
}
#endif

int Popcount32(uint32_t x) {
    static_assert(sizeof(unsigned int) == sizeof(uint32_t),
                  "unsigned int is not 32 bits");
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(x);
#elif defined(_MSC_VER)
    return __popcnt(x);
#else
    return FallbackPopcount32(x);
#endif
}

int Popcount64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(x);
#elif defined(_MSC_VER) && defined(_M_X64)
    return __popcnt64(x);
#elif defined(_MSC_VER)  // 32-bit fallback for MSVC
    return __popcnt((uint32_t)x) + __popcnt((uint32_t)(x >> 32));
#else
    return FallbackPopcount64(x);
#endif
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

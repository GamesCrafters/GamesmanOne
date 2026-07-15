/**
 * @file xfile.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief File operation wrapper functions implementation.
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

#include "libs/io/xfile.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zconf.h>
#include <zlib.h>

FILE *GuardedFopen(const char *filename, const char *modes) {
    FILE *f = fopen(filename, modes);
    if (f == NULL) {
        perror("fopen");
    }

    return f;
}

FILE *GuardedFreopen(const char *filename, const char *modes, FILE *stream) {
    FILE *ret = freopen(filename, modes, stream);
    if (ret == NULL) {
        perror("freopen");
    }

    return ret;
}

int GuardedFclose(FILE *stream) {
    int error = fclose(stream);
    if (error != 0) {
        perror("fclose");
    }

    return error;
}

int GuardedOpen(const char *filename, int flags) {
    int fd = open(filename, flags);
    if (fd == -1) {
        perror("open");
    }

    return fd;
}

int GuardedClose(int fd) {
    int error = close(fd);
    if (error == -1) {
        perror("close");
    }

    return error;
}

int GuardedRename(const char *oldpath, const char *newpath) {
    int error = rename(oldpath, newpath);
    if (error == -1) {
        perror("rename");
    }

    return error;
}

int GuardedRemove(const char *pathname) {
    int error = remove(pathname);
    if (error == -1) {
        perror("remove");
    }

    return error;
}

gzFile GuardedGzdopen(int fd, const char *mode) {
    gzFile file = gzdopen(fd, mode);
    if (file == Z_NULL) {
        perror("gzdopen");
    }

    return file;
}

int GuardedGzclose(gzFile file) {
    int error = gzclose(file);
    if (error != Z_OK) {
        perror("gzclose");
    }

    return error;
}

int GuardedGzread(gzFile file, voidp buf, unsigned int length, bool eof_ok) {
    int bytes_read = gzread(file, buf, length);
    if ((unsigned int)bytes_read == length) {
        return 0;
    }

    int error;
    if (gzeof(file)) {
        if (eof_ok) {
            return 0;
        }
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

    fprintf(stderr, "GuardedGzread: unknown error occurred during gzread()");
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

bool FileExists(const char *filename) {
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
 * @return 0 on success. On error, 1 is returned and errno is set to indicate
 * the error.
 *
 * @author Jonathon Reinhart
 * @link
 * https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
 */
static int MaybeMkdir(const char *path, mode_t mode) {
    errno = 0;

    // Try to make the directory
    if (mkdir(path, mode) == 0) {
        return 0;
    }

    // If it fails for any reason but EEXIST, fail
    if (errno != EEXIST) {
        return 1;
    }

    // Check if the existing path is a directory
    struct stat st;
    if (stat(path, &st) != 0) {
        return 1;
    }

    // If not, fail with ENOTDIR
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return 1;
    }

    errno = 0;
    return 0;
}

MkdirRecursiveStatus MkdirRecursive(const char *path) {
    // Fail if path is NULL.
    if (path == NULL) {
        errno = EINVAL;
        return kMkdirRecursiveErrInvalidParam;
    }

    int ret = kMkdirRecursiveSuccess;
    errno = 0;

    // Copy string so it's mutable
    size_t path_length = strlen(path);
    char *path_copy = (char *)malloc(path_length + 1);
    if (path_copy == NULL) {
        ret = kMkdirRecursiveErrOom;
        errno = ENOMEM;
        goto _bailout;
    }
    strcpy(path_copy, path);

    // Start at i = 1 to handle a single leading slash.
    for (size_t i = 1; i < path_length; ++i) {
        // Only trigger if we hit a slash AND the previous character wasn't a
        // slash. This safely skips over consecutive slashes (e.g., "//" or
        // "///").
        if (path_copy[i] == '/' && path_copy[i - 1] != '/') {
            path_copy[i] = '\0';  // Temporarily truncate
            if (MaybeMkdir(path_copy, 0777) != 0) {
                ret = kMkdirRecursiveErrFileSystem;
                goto _bailout;
            }
            path_copy[i] = '/';
        }
    }
    if (MaybeMkdir(path_copy, 0777) != 0) {
        ret = kMkdirRecursiveErrFileSystem;
        goto _bailout;
    }

_bailout:
    free(path_copy);
    return ret;
}

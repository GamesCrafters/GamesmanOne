/**
 * @file xfile.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Library for file operation wrapper functions.
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

#ifndef GAMESMANONE_LIBS_IO_XFILE_H_
#define GAMESMANONE_LIBS_IO_XFILE_H_

#include <stdbool.h>
#include <stdio.h>
#include <zconf.h>
#include <zlib.h>

/**
 * @brief Same behavior as fopen on success; calls perror and returns NULL
 * otherwise.
 * Reference: https://man7.org/linux/man-pages/man3/fopen.3.html
 */
FILE *GuardedFopen(const char *filename, const char *modes);

/**
 * @brief Same behavior as freopen on success; calls perror and returns NULL
 * otherwise.
 * Reference: https://man7.org/linux/man-pages/man3/freopen.3p.html
 */
FILE *GuardedFreopen(const char *filename, const char *modes, FILE *stream);

/**
 * @brief Same behavior as fclose on success; calls perror and returns a
 * non-zero error code otherwise.
 * Reference: https://man7.org/linux/man-pages/man3/fclose.3.html
 */
int GuardedFclose(FILE *stream);

/**
 * @brief Same behavior as open on success; calls perror and returns -1
 * otherwise.
 * Reference: https://man7.org/linux/man-pages/man2/open.2.html
 */
int GuardedOpen(const char *filename, int flags);

/**
 * @brief Same behavior as close on success; calls perror and returns -1
 * otherwise.
 * Reference: https://man7.org/linux/man-pages/man2/close.2.html
 */
int GuardedClose(int fd);

/**
 * @brief Same behavior as rename on success; calls perror and returns -1
 * otherwise.
 * Reference: https://man7.org/linux/man-pages/man2/rename.2.html
 */
int GuardedRename(const char *oldpath, const char *newpath);

/**
 * @brief Same behavior as remove on success; calls perror and returns -1
 * otherwise.
 * Reference: https://man7.org/linux/man-pages/man3/remove.3.html
 */
int GuardedRemove(const char *pathname);

/**
 * @brief Same behavior as gzdopen on success; calls perror and returns Z_NULL
 * otherwise.
 * Reference:
 * https://refspecs.linuxbase.org/LSB_3.0.0/LSB-Core-generic/LSB-Core-generic/zlib-gzdopen-1.html
 */
gzFile GuardedGzdopen(int fd, const char *mode);

/**
 * @brief Same behavior as gzclose on success; calls perror and returns the
 * non-zero error code returned by gzclose otherwise.
 * Reference:
 * https://refspecs.linuxbase.org/LSB_3.0.0/LSB-PDA/LSB-PDA/zlib-gzclose-1.html
 */
int GuardedGzclose(gzFile file);

/**
 * @brief Calls gzread with the given FILE, BUF, and LENGTH and returns 0 if the
 * correct number of bytes are read or EOF_OK is set to true; returns a non-zero
 * error code otherwise.
 * Reference:
 * https://refspecs.linuxbase.org/LSB_3.0.0/LSB-Core-generic/LSB-Core-generic/zlib-gzread-1.html
 *
 * @param file Source gzFile.
 * @param buf Destination buffer, which is assumed to be of size at least LENGTH
 * bytes.
 * @param length Number of uncompressed bytes to read from FILE.
 * @param eof_ok Whether end-of-file is accepted as no error. Set this to false
 * if you expect FILE to contain at least LENGTH bytes of uncompressed data.
 * @return 0 on success, 2 if EOF_OK is set to false and EOF is reached before
 * LENGTH bytes are read, or 3 if gzerror returns an error on FILE.
 */
int GuardedGzread(gzFile file, voidp buf, unsigned int length, bool eof_ok);

/**
 * @brief Calls gzwrite with the given FILE, BUF, and LEN and returns 0 if the
 * correct number of bytes are written; returns the error value returned by
 * gzerror otherwise.
 * Reference:
 * https://refspecs.linuxbase.org/LSB_3.0.0/LSB-Core-generic/LSB-Core-generic/zlib-gzwrite-1.html
 */
int GuardedGzwrite(gzFile file, voidpc buf, unsigned int len);

/**
 * @brief Returns true if the file with the given \p filename exists, or false
 * otherwise.
 */
bool FileExists(const char *filename);

typedef enum {
    kMkdirRecursiveSuccess,
    kMkdirRecursiveErrInvalidParam,
    kMkdirRecursiveErrFileSystem,
    kMkdirRecursiveErrOom,
} MkdirRecursiveStatus;

/**
 * @brief Recursively makes all directories along the given path.
 * Equivalent to "mkdir -p <path>".
 *
 * @param path Make all directories along this path.
 * @return 0 on success. On error, a non-zero value is returned and errno is set
 * to indicate the error.
 *
 * @authors Jonathon Reinhart and Carl Norum
 * Reference: http://stackoverflow.com/a/2336245/119527,
 * https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
 */
MkdirRecursiveStatus MkdirRecursive(const char *path);

#endif  // GAMESMANONE_LIBS_IO_XFILE_H_

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
 * @brief Wrapper for `fopen`. Calls `perror` and returns `NULL` on failure.
 *
 * @param[in] filename Path to the file to open.
 * @param[in] modes Mode string to open the file with.
 *
 * @return A pointer to the opened file, or `NULL` on error.
 *
 * @see https://man7.org/linux/man-pages/man3/fopen.3.html
 */
FILE *GuardedFopen(const char *filename, const char *modes);

/**
 * @brief Wrapper for `freopen`. Calls `perror` and returns `NULL` on failure.
 *
 * @param[in] filename Path to the file to open.
 * @param[in] modes Mode string to open the file with.
 * @param[in,out] stream Stream to reopen.
 *
 * @return A pointer to the reopened file, or `NULL` on error.
 *
 * @see https://man7.org/linux/man-pages/man3/freopen.3p.html
 */
FILE *GuardedFreopen(const char *filename, const char *modes, FILE *stream);

/**
 * @brief Wrapper for `fclose`. Calls `perror` on failure.
 *
 * @param[in,out] stream File stream to close.
 *
 * @return `0` on success, or `EOF` (non-zero) on error.
 *
 * @see https://man7.org/linux/man-pages/man3/fclose.3.html
 */
int GuardedFclose(FILE *stream);

/**
 * @brief Wrapper for `open`. Calls `perror` and returns `-1` on failure.
 *
 * @param[in] filename Path to the file to open.
 * @param[in] flags File creation and status flags.
 *
 * @return The new file descriptor, or `-1` on error.
 *
 * @see https://man7.org/linux/man-pages/man2/open.2.html
 */
int GuardedOpen(const char *filename, int flags);

/**
 * @brief Wrapper for `close`. Calls `perror` and returns `-1` on failure.
 *
 * @param[in] fd File descriptor to close.
 *
 * @return `0` on success, or `-1` on error.
 *
 * @see https://man7.org/linux/man-pages/man2/close.2.html
 */
int GuardedClose(int fd);

/**
 * @brief Wrapper for `rename`. Calls `perror` and returns `-1` on failure.
 *
 * @param[in] oldpath Current path of the file.
 * @param[in] newpath New path for the file.
 *
 * @return `0` on success, or `-1` on error.
 *
 * @see https://man7.org/linux/man-pages/man2/rename.2.html
 */
int GuardedRename(const char *oldpath, const char *newpath);

/**
 * @brief Wrapper for `remove`. Calls `perror` and returns `-1` on failure.
 *
 * @param[in] pathname Path to the file or directory to remove.
 *
 * @return `0` on success, or `-1` on error.
 *
 * @see https://man7.org/linux/man-pages/man3/remove.3.html
 */
int GuardedRemove(const char *pathname);

/**
 * @brief Wrapper for `gzdopen`. Calls `perror` and returns `Z_NULL` on failure.
 *
 * @param[in] fd File descriptor to associate with the gzip stream.
 * @param[in] mode Mode string to open the stream with.
 *
 * @return A pointer to the opened gzip file, or `Z_NULL` on error.
 *
 * @see
 * https://refspecs.linuxbase.org/LSB_3.0.0/LSB-Core-generic/LSB-Core-generic/zlib-gzdopen-1.html
 */
gzFile GuardedGzdopen(int fd, const char *mode);

/**
 * @brief Wrapper for `gzclose`. Calls `perror` on failure.
 *
 * @param[in,out] file Gzip file stream to close.
 *
 * @return `Z_OK` (`0`) on success, or a non-zero error code on error.
 *
 * @see
 * https://refspecs.linuxbase.org/LSB_3.0.0/LSB-PDA/LSB-PDA/zlib-gzclose-1.html
 */
int GuardedGzclose(gzFile file);

/**
 * @brief Calls `gzread` and verifies the correct number of bytes are read.
 *
 * @param[in,out] file Source gzip file stream.
 * @param[out] buf Destination buffer, assumed to be at least `length` bytes.
 * @param[in] length Number of uncompressed bytes to read from `file`.
 * @param[in] eof_ok Whether end-of-file is accepted as no error. Set to `false`
 * if `file` is expected to contain at least `length` bytes.
 *
 * @retval 0 Success, or `eof_ok` is `true` and EOF is reached.
 * @retval 2 `eof_ok` is `false` and EOF is reached before `length` bytes.
 * @retval 3 `gzerror` returns an error on `file`.
 * @retval 4 An unknown error occurred.
 *
 * @see
 * https://refspecs.linuxbase.org/LSB_3.0.0/LSB-Core-generic/LSB-Core-generic/zlib-gzread-1.html
 */
int GuardedGzread(gzFile file, voidp buf, unsigned int length, bool eof_ok);

/**
 * @brief Calls `gzwrite` and verifies the correct number of bytes are written.
 *
 * @param[in,out] file Destination gzip file stream.
 * @param[in] buf Source buffer to write from.
 * @param[in] len Number of uncompressed bytes to write.
 *
 * @return `0` on success, or the error value returned by `gzerror` otherwise.
 *
 * @see
 * https://refspecs.linuxbase.org/LSB_3.0.0/LSB-Core-generic/LSB-Core-generic/zlib-gzwrite-1.html
 */
int GuardedGzwrite(gzFile file, voidpc buf, unsigned int len);

/**
 * @brief Checks if a file exists and is readable.
 *
 * @param[in] filename Path to the file to check.
 *
 * @return `true` if the file exists and can be opened for reading, `false`
 * otherwise.
 */
bool FileExists(const char *filename);

/**
 * @brief Status codes returned by `MkdirRecursive`.
 */
typedef enum {
    kMkdirRecursiveSuccess,         /**< Directory created successfully. */
    kMkdirRecursiveErrInvalidParam, /**< Invalid parameter provided. */
    kMkdirRecursiveErrFileSystem,   /**< File system error occurred. */
    kMkdirRecursiveErrOom,          /**< Out of memory error. */
} MkdirRecursiveStatus;

/**
 * @brief Recursively makes all directories along the given path.
 *
 * @details Equivalent to running `mkdir -p <path>`.
 *
 * @param[in] path Make all directories along this path. Empty strings are
 * accepted, in which case no directories will be created.
 *
 * @return A `MkdirRecursiveStatus` indicating success or the specific error.
 *
 * @copyright
 * Authors: Jonathon Reinhart and Carl Norum
 * Source: http://stackoverflow.com/a/2336245/119527
 * Source:
 * https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
 */
MkdirRecursiveStatus MkdirRecursive(const char *path);

#endif  // GAMESMANONE_LIBS_IO_XFILE_H_

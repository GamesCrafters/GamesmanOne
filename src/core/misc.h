/**
 * @file misc.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Miscellaneous utility functions.
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

#ifndef GAMESMANONE_CORE_MISC_H_
#define GAMESMANONE_CORE_MISC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <zconf.h>
#include <zlib.h>

#ifdef USE_MPI
#include <mpi.h>
#endif  // USE_MPI

#include "core/types/base.h"

/**
 * @brief Gracefully exits GAMESMAN.
 * @warning This function calls exit() which is not MT-safe. Do not call this
 * function in a multithreaded code section.
 */
void GamesmanExit(void);

/** @brief Prints the error MESSAGE and terminates GAMESMAN. */
void NotReached(ReadOnlyString message);

/**
 * @brief Same behavior as strncpy if the end of SRC is found before N
 * characters are copied. Otherwise, copies N-1 characters from SRC to DEST and
 * terminates DEST with a null-terminator. Therefore, DEST will always be a null
 * terminated C string.
 *
 * @param dest Destination buffer, which is assumed to be of size at least N.
 * @param src Source buffer.
 * @param n Number of characters to copy into DEST.
 * @return DEST.
 */
char *SafeStrncpy(char *dest, const char *src, size_t n);

/**
 * @brief Equivalent to first calling printf with the given parameters and then
 * calling fflush(stdout).
 */
void PrintfAndFlush(const char *format, ...);

/**
 * @brief Prints \p prompt followed by a new line ('\n') and an arrow ("=>") to
 * \c stdout, and then reads in X characters from \c stdin until a new line or
 * EOF is encountered but only writes up to \p length_max characters to \p buf,
 * not including the trailing new line character ('\n').
 *
 * @note \p buf is assumed to have enough space to hold at least \p length_max +
 * 1 characters to include the terminal '\0'.
 *
 * @param prompt A prompt to be printed out that explains what the user input
 * should be.
 * @param buf Output parameter. The user input, up to \p length_max bytes, is
 * stored in the contiguous space that this pointer is pointing to.
 * @param length_max Maximum acceptable user input length in number of
 * characters.
 * @return \p buf.
 */
char *PromptForInput(ReadOnlyString prompt, char *buf, int length_max);

/** @brief Return the current system time stamp as a c-string. */
char *GetTimeStampString(void);

/**
 * @brief Returns the time equivalent to SECONDS seconds in the format of "[YYYY
 * y MM m DD d HH h MM m ]SS s" as a c-string.
 */
char *SecondsToFormattedTimeString(double seconds);

/**
 * @brief Same behavior as fopen on success; calls perror and returns NULL
 * otherwise.
 */
FILE *GuardedFopen(const char *filename, const char *modes);

/**
 * @brief Same behavior as freopen on success; calls perror and returns NULL
 * otherwise.
 */
FILE *GuardedFreopen(const char *filename, const char *modes, FILE *stream);

/**
 * @brief Same behavior as fclose on success; calls perror and returns a
 * non-zero error code otherwise.
 */
int GuardedFclose(FILE *stream);

/**
 * @brief Same behavior as fseek on success; calls perror and returns the error
 * code returned by fseek, which is always -1.
 * Reference: https://man7.org/linux/man-pages/man3/fseek.3.html
 */
int GuardedFseek(FILE *stream, long off, int whence);

/**
 * @brief Calls fread and returns 0 on success; prints out the error occurred
 * and returns a non-zero error code otherwise.
 * @details The fread function never sets errno and therefore perror does not
 * generate helpful error messages.
 * Reference: https://man7.org/linux/man-pages/man3/fread.3.html
 *
 * @return 0 on success; returns 2 if EOF is reached before N items are read, or
 * 3 if there is an error with STREAM.
 */
int GuardedFread(void *ptr, size_t size, size_t n, FILE *stream, bool eof_ok);

/**
 * @brief Calls fwrite and returns 0 on success; calls perror and returns errno
 * otherwise.
 */
int GuardedFwrite(const void *ptr, size_t size, size_t n, FILE *stream);

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
 */
gzFile GuardedGzdopen(int fd, const char *mode);

/**
 * @brief Same behavior as gzclose on success; calls perror and returns the
 * non-zero error code returned by gzclose otherwise.
 */
int GuardedGzclose(gzFile file);

/**
 * @brief Calls gzread with the given FILE, BUF, and LENGTH and returns 0 if the
 * correct number of bytes are read or EOF_OK is set to true; returns a non-zero
 * error code otherwise.
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
 */
int GuardedGzwrite(gzFile file, voidpc buf, unsigned int len);

/**
 * @brief Returns true if the file with the given \p filename exists, or false
 * otherwise.
 */
bool FileExists(ReadOnlyString filename);

/**
 * @brief Recursively makes all directories along the given path.
 * Equivalent to "mkdir -p <path>".
 *
 * @param path Make all directories along this path.
 * @return 0 on success. On error, -1 is returned and errno is set to indicate
 * the error.
 *
 * @authors Jonathon Reinhart and Carl Norum
 * Reference: http://stackoverflow.com/a/2336245/119527,
 * https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
 */
int MkdirRecursive(ReadOnlyString path);

#ifdef USE_MPI

/**
 * @brief Bail-on-error \c MPI_Init_thread.
 *
 * @param argc Pointer to the number of arguments.
 * @param argv Pointer to the argument vector.
 * @param required Level of desired thread support.
 * @param provided (Output parameter) level of provided thread support.
 */
void SafeMpiInitThread(int *argc, char ***argv, int required, int *provided);

/**
 * @brief Bail-on-error \c MPI_Init.
 *
 * @param argc Pointer to the number of arguments.
 * @param argv Pointer to the argument vector.
 */
void SafeMpiInit(int *argc, char ***argv);

/**
 * @brief Bail-on-error \c MPI_Finalize.
 */
void SafeMpiFinalize(void);

/**
 * @brief Bail-on-error \c MPI_Comm_size.
 *
 * @param comm Communicator (handle).
 * @return Number of processes in the group of \p comm.
 */
int SafeMpiCommSize(MPI_Comm comm);

/**
 * @brief Bail-on-error \c MPI_Comm_rank.
 *
 * @param comm Communicator (handle).
 * @return Rank of the calling process in the group of \c comm.
 */
int SafeMpiCommRank(MPI_Comm comm);

/**
 * @brief Bail-on-error \c MPI_Send.
 *
 * @param buf Initial address of send buffer (choice).
 * @param count Number of elements in send buffer (non-negative integer).
 * @param datatype Datatype of each send buffer element (handle).
 * @param dest Rank of destination (integer).
 * @param tag Message tag (integer).
 * @param comm Communicator (handle).
 */
void SafeMpiSend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                 MPI_Comm comm);

/**
 * @brief Bail-on-error \c MPI_Recv.
 *
 * @param buf (Output parameter) initial address of receive buffer (choice).
 * @param count Communicator (handle).
 * @param datatype Maximum number of elements in receive buffer (integer).
 * @param source Datatype of each receive buffer element (handle).
 * @param tag Rank of source (integer).
 * @param comm Message tag (integer).
 * @param status (Output parameter) status object.
 */
void SafeMpiRecv(void *buf, int count, MPI_Datatype datatype, int source,
                 int tag, MPI_Comm comm, MPI_Status *status);
#endif  // USE_MPI

#endif  // GAMESMANONE_CORE_MISC_H_

/**
 * @file misc.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Miscellaneous utility functions.
 * @version 1.4.0
 * @date 2024-11-09
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

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // int64_t, uint64_t
#include <stdio.h>    // FILE
#include <time.h>     // clock_t
#include <zlib.h>     // gzFile

#ifdef USE_MPI
#include <mpi.h>
#endif  // USE_MPI

#include "core/types/gamesman_types.h"

/** @brief Exits GAMESMAN. */
void GamesmanExit(void);

/** @brief Prints the error MESSAGE and terminates GAMESMAN. */
void NotReached(ReadOnlyString message);

/**
 * @brief Returns the amount of physical memory available on the system.
 *
 * @return Amount of physical memory or
 * @return 0 if the detection fails.
 */
intptr_t GetPhysicalMemory(void);

/**
 * @brief Same behavior as malloc() on success; terminates GAMESMAN on failure.
 */
void *SafeMalloc(size_t size);

/**
 * @brief Same behavior as calloc() on success; terminates GAMESMAN on failure.
 */
void *SafeCalloc(size_t n, size_t size);

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

/**
 * @brief Adds the given byte OFFSET to the given generic pointer P. Return
 * value is equivalent to (void *)((uint8_t *)P + OFFSET)).
 *
 * @param p Generic pointer.
 * @param offset Byte offset.
 * @return Shifted pointer (void *)((uint8_t *)P + OFFSET)).
 */
void *GenericPointerAdd(const void *p, int64_t offset);

/** @brief Return the number of seconds corresponding to N clock ticks. */
double ClockToSeconds(clock_t n);

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
 * @brief Calls fclose on STREAM and returns error.
 * @details This function is typically called when an error occurred in the
 * middle of a function call and the function needs to close the given STREAM
 * before returning. This function will close the STREAM, and return the ERROR
 * code so that the caller of this function can save one line of code by
 * writting "return BailOutFclose(stream, error);" as the return statement,
 * instead of "fclose(stream); return error;".
 *
 * @param stream FILE to close.
 * @param error Error code to be returned by this function.
 * @return ERROR.
 */
int BailOutFclose(FILE *stream, int error);

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
 * @brief Calls close on FD and returns error.
 * @details This function is typically called when an error occurred in the
 * middle of a function call and the function needs to close the given FD before
 * returning. This function will close the FD, and return the ERROR code so
 * that the caller of this function can save one line of code by writting
 * "return BailOutClose(fd, error);" as the return statement, instead of
 * "close(fd); return error;".
 *
 * @param fd File descriptor to close.
 * @param error Error code to be returned by this function.
 * @return ERROR.
 */
int BailOutClose(int fd, int error);

/**
 * @brief Calls lseek and returns 0 if the value returned by lseek matches
 * OFFSET; calls perror and returns -1 otherwise.
 * Reference: https://man7.org/linux/man-pages/man2/lseek.2.html
 */
int GuardedLseek(int fd, off_t offset, int whence);

/**
 * @brief Same behavior as gzopen on success; calls perror and returns Z_NULL
 * otherwise.
 */
gzFile GuardedGzopen(const char *path, const char *mode);

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
 * @brief Calls gzclose on FILE and returns error.
 * @details This function is typically called when an error occurred in the
 * middle of a function call and the function needs to close the given FILE
 * before returning. This function will close the FILE, and return the ERROR
 * code so that the caller of this function can save one line of code by
 * writting "return BailOutGzclose(file, error);" as the return statement,
 * instead of "gzclose(file); return error;".
 *
 * @param file gzFile to close.
 * @param error Error code to be returned by this function.
 * @return ERROR.
 */
int BailOutGzclose(gzFile file, int error);

/**
 * @brief Calls gzseek and returns 0 if the value returned by gzseek matches
 * OFF; calls perror and returns -1 otherwise.
 */
int GuardedGzseek(gzFile file, off_t off, int whence);

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
 * @brief Calls gz64_read with the given FILE, BUF, and LENGTH and returns 0 if
 * the correct number of bytes are read or EOF_OK is set to true; returns a
 * non-zero error code otherwise.
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
int GuardedGz64Read(gzFile file, voidp buf, uint64_t length, bool eof_ok);

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

/**
 * @brief Tests if N is prime. Returns false if N is non-possitive.
 *
 * @param n Integer.
 * @return True if N is a positive prime number, false otherwise.
 * @author Naman_Garg, geeksforgeeks.org.
 * https://www.geeksforgeeks.org/program-to-find-the-next-prime-number/
 */
bool IsPrime(int64_t n);

/**
 * @brief Returns the largest prime number that is smaller than
 * or equal to N, unless N is less than 2, in which case 2 is
 * returned.
 *
 * @param n Reference.
 * @return Previous prime of N.
 */
int64_t PrevPrime(int64_t n);

/**
 * @brief Returns the smallest prime number that is greater than
 * or equal to N, assuming no integer overflow occurs.
 *
 * @param n Reference.
 * @return Next prime of N.
 */
int64_t NextPrime(int64_t n);

/**
 * @brief Returns the next multiple of MULTIPLE starting from N. Returns
 * N if N is a multiple of MULTIPLE.
 */
int64_t NextMultiple(int64_t n, int64_t multiple);

/**
 * @brief Returns a+b, or -1 if either a or b is negative or if a+b overflows.
 */
int64_t SafeAddNonNegativeInt64(int64_t a, int64_t b);

/**
 * @brief Returns a+b, or -1 if either a or b is negative or if a+b overflows.
 */
int64_t SafeMultiplyNonNegativeInt64(int64_t a, int64_t b);

/**
 * @brief Returns the number of ways to choose R elements from a total of N
 * elements.
 *
 * @param n Positive integer, number of elements to choose from.
 * @param r Positive integer, number of elements to choose.
 * @return Returns nCr(N, R) if the result can be expressed as a 64-bit
 * signed integer. Returns -1 if either N or R is negative or if the result
 * overflows.
 */
int64_t NChooseR(int n, int r);

/**
 * @brief Returns N / D if D divides N; returns N / D + 1 otherwise.
 * @warning D must not be 0.
 */
int64_t RoundUpDivide(int64_t n, int64_t d);

#ifdef USE_MPI
/**
 * @brief Bail-on-error \c MPI_Init.
 *
 * @param argc Pointer to the number of arguments.
 * @param argv Pointer to the argument vector.
 */
void SafeMpiInit(int *argc, char ***argv);

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

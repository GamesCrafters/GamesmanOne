/**
 * @file misc.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Declaration of miscellaneous utility functions.
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

#ifndef GAMESMANEXPERIMENT_CORE_MISC_H_
#define GAMESMANEXPERIMENT_CORE_MISC_H_

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // int64_t
#include <stdio.h>    // FILE
#include <zlib.h>     // gzFile

#include "core/gamesman_types.h"

/** @brief Exits GAMESMAN. */
void GamesmanExit(void);

/** @brief Prints the error MESSAGE and terminates GAMESMAN. */
void NotReached(ReadOnlyString message);

/**
 * @brief Same behavior as malloc() on success; terminates GAMESMAN on failure.
 */
void *SafeMalloc(size_t size);

/**
 * @brief Same behavior as calloc() on success; terminates GAMESMAN on failure.
 */
void *SafeCalloc(size_t n, size_t size);

char *SafeStrncpy(char *dest, const char *src, size_t n);

void *GenericPointerAdd(const void *p, int64_t offset);

FILE *GuardedFopen(const char *filename, const char *modes);

int GuardedFclose(FILE *stream);

int BailOutFclose(FILE *stream, int error);

int GuardedFseek(FILE *stream, long off, int whence);

int GuardedFread(void *ptr, size_t size, size_t n, FILE *stream);

int GuardedFwrite(const void *ptr, size_t size, size_t n, FILE *stream);

int GuardedOpen(const char *filename, int flags);

int GuardedClose(int fd);

int BailOutClose(int fd, int error);

int GuardedLseek(int fd, off_t offset, int whence);

gzFile GuardedGzdopen(int fd, const char *mode);

int GuardedGzclose(gzFile file);

int BailOutGzclose(gzFile file, int error);

int GuardedGzseek(gzFile file, off_t off, int whence);

int GuardedGzread(gzFile file, voidp buf, unsigned int length, bool eof_ok);

/**
 * @brief Recursively makes all directories along the given path.
 * Equivalent to "mkdir -p <path>".
 *
 * @param path Make all directories along this path.
 * @return 0 on success. On error, -1 is returned and errno is set to indicate
 * the error.
 *
 * @authors Jonathon Reinhart and Carl Norum
 * @link http://stackoverflow.com/a/2336245/119527,
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

#endif  // GAMESMANEXPERIMENT_CORE_MISC_H_

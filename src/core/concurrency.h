/**
 * @file concurrency.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief A convenience library for OpenMP pragmas and concurrent data type
 * definitions that work in both single-threaded and multithreaded GamesmanOne
 * builds.
 * @version 1.2.0
 * @date 2025-05-27
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

#ifndef GAMESMANONE_CORE_CONCURRENCY_H_
#define GAMESMANONE_CORE_CONCURRENCY_H_

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t

#ifdef _OPENMP

#include <stdatomic.h>  // atomic_bool, atomic_int

#define PRAGMA(X) _Pragma(#X)
#define PRAGMA_OMP(expression) PRAGMA(omp expression)

typedef atomic_bool ConcurrentBool;
typedef atomic_int ConcurrentInt;
typedef atomic_size_t ConcurrentSizeType;

#else  // _OPENMP not defined.

// The following macros do nothing.

#define PRAGMA_OMP(expression)

typedef bool ConcurrentBool;
typedef int ConcurrentInt;
typedef size_t ConcurrentSizeType;

#endif  // _OPENMP

/**
 * @brief Initializes the ConcurrentBool variable at \p cb to \p val.
 *
 * @param cb Pointer to the ConcurrentBool to initialize.
 * @param val Value to initialize to.
 */
void ConcurrentBoolInit(ConcurrentBool *cb, bool val);

/**
 * @brief Returns the current value of the ConcurrentBool variable at \p cb.
 * In a multithreaded context, the load operation is atomic with default memory
 * order.
 *
 * @param cb Pointer to the ConcurrentBool variable to load the value from.
 * @return The current value of the ConcurrentBool variable.
 */
bool ConcurrentBoolLoad(const ConcurrentBool *cb);

/**
 * @brief Stores the value \p val into the ConcurrentBool variable at \p cb.
 * In a multithreaded context, the store operation is atomic with default memory
 * order.
 *
 * @param cb Pointer to the ConcurrentBool variable to store the value into.
 * @return The value to store.
 */
void ConcurrentBoolStore(ConcurrentBool *cb, bool val);

/**
 * @brief Initializes the ConcurrentInt variable at \p ci to \p val.
 *
 * @param ci Pointer to the ConcurrentInt to initialize.
 * @param val Value to initialize to.
 */
void ConcurrentIntInit(ConcurrentInt *ci, int val);

/**
 * @brief Returns the current value of the ConcurrentInt variable at \p ci.
 * In a multithreaded context, the load operation is atomic with default memory
 * order.
 *
 * @param ci Pointer to the ConcurrentInt variable to load the value from.
 * @return The current value of the ConcurrentInt variable.
 */
int ConcurrentIntLoad(const ConcurrentInt *ci);

/**
 * @brief Stores the value \p val into the ConcurrentInt variable at \p ci.
 * In a multithreaded context, the store operation is atomic with default memory
 * order.
 *
 * @param ci Pointer to the ConcurrentInt variable to store the value into.
 * @return The value to store.
 */
void ConcurrentIntStore(ConcurrentInt *ci, int val);

/**
 * @brief Replaces the value pointed by \p ci with the maximum of its original
 * value and \p val and returns the original value.
 *
 * @param ci Pointer to the ConcurrentInt variable to store the maximum value
 * into.
 * @param val Another value.
 * @return The value pointed by \p ci immediately preceding the effects of this
 * function.
 */
int ConcurrentIntMax(ConcurrentInt *ci, int val);

/**
 * @brief Initializes the ConcurrentSizeType variable at \p cs to \p val.
 *
 * @param cs Pointer to the ConcurrentSizeType to initialize.
 * @param val Value to initialize to.
 */
void ConcurrentSizeTypeInit(ConcurrentSizeType *cs, size_t val);

/**
 * @brief Returns the current value of the ConcurrentSizeType variable at \p cs.
 * In a multithreaded context, the load operation is atomic with default memory
 * order.
 *
 * @param cs Pointer to the ConcurrentSizeType variable to load the value from.
 * @return The current value of the ConcurrentSizeType variable.
 */
size_t ConcurrentSizeTypeLoad(const ConcurrentSizeType *cs);

/**
 * @brief Subtracts \p val from the ConcurrentSizeType variable at \p cs if and
 * only if its current value is greater than or equal to \p val. In a
 * multithreaded context, the subtraction is atomic with default memory order.
 *
 * @param cs Pointer to the ConcurrentSizeType variable to subtract the value
 * from.
 * @param val Value to subtract.
 * @return \c true if the subtraction is successfully completed, or
 * @return \c false if the current value of \p cs is smaller than \p val.
 */
bool ConcurrentSizeTypeSubtractIfGreaterEqual(ConcurrentSizeType *cs,
                                              size_t val);

/**
 * @brief Adds \p val to the ConcurrentSizeType variable at \p cs and returns
 * its original value. In a multithreaded context, the addition is atomic with
 * default memory order.
 *
 * @param cs Pointer to the ConcurrentSizeType variable to add the value to.
 * @param val Value to add.
 * @return The original value in \p cs before the addition.
 */
size_t ConcurrentSizeTypeAdd(ConcurrentSizeType *cs, size_t val);

/**
 * @brief Subtracts \p val to the ConcurrentSizeType variable at \p cs and
 * returns its original value. In a multithreaded context, the addition is
 * atomic with default memory order.
 *
 * @param cs Pointer to the ConcurrentSizeType variable to subtract the value
 * from.
 * @param val Value to add.
 * @return The original value in \p cs before the subtraction.
 */
size_t ConcurrentSizeTypeSubtract(ConcurrentSizeType *cs, size_t val);

/**
 * @brief Returns the number of OpenMP threads available. Returns 1 if
 * OpenMP is disabled.
 *
 * @return Number of OpenMP threads available.
 */
int ConcurrencyGetOmpNumThreads(void);

/**
 * @brief Returns the caller's OpenMP thread ID. Returns 0 if OpenMP is
 * disabled.
 *
 * @return The caller's OpenMP thread ID.
 */
int ConcurrencyGetOmpThreadId(void);

#endif  // GAMESMANONE_CORE_CONCURRENCY_H_

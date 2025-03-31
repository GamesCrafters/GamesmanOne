/**
 * @file concurrency.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief A convenience library for OpenMP pragmas and concurrent data type
 * definitions that work in both single-threaded and multithreaded GamesmanOne
 * builds.
 * @version 1.1.0
 * @date 2025-03-30
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

#ifdef _OPENMP

#include <stdatomic.h>  // atomic_bool, atomic_int

#define PRAGMA(X) _Pragma(#X)

#define PRAGMA_OMP_PARALLEL PRAGMA(omp parallel)
#define PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(k) PRAGMA(omp for schedule(dynamic, k))

#define PRAGMA_OMP_PARALLEL_FOR PRAGMA(omp parallel for)
#define PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(k) PRAGMA(omp parallel for schedule(dynamic, k))

#define PRAGMA_OMP_CRITICAL(name) PRAGMA(omp critical(name))

typedef atomic_bool ConcurrentBool;
typedef atomic_int ConcurrentInt;

#else  // _OPENMP not defined.

// The following macros do nothing.

#define PRAGMA_OMP_PARALLEL
#define PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(k)

#define PRAGMA_OMP_PARALLEL_FOR
#define PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(k)

#define PRAGMA_OMP_CRITICAL(name)

typedef bool ConcurrentBool;
typedef int ConcurrentInt;

#endif  // _OPENMP

void ConcurrentBoolInit(ConcurrentBool *cb, bool val);
bool ConcurrentBoolLoad(const ConcurrentBool *cb);
void ConcurrentBoolStore(ConcurrentBool *cb, bool val);

void ConcurrentIntInit(ConcurrentInt *ci, int val);
int ConcurrentIntLoad(const ConcurrentInt *ci);
void ConcurrentIntStore(ConcurrentInt *ci, int val);

int ConcurrencyGetOmpNumThreads(void);
int ConcurrencyGetOmpThreadId(void);

#endif  // GAMESMANONE_CORE_CONCURRENCY_H_

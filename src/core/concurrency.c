/**
 * @file concurrency.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the concurrency convenience library.
 * @version 1.1.0
 * @date 2025-04-23
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

#include "core/concurrency.h"

#ifdef _OPENMP
#include <omp.h>
#include <stdatomic.h>
#endif

#include <stdbool.h>

void ConcurrentBoolInit(ConcurrentBool *cb, bool val) {
#ifdef _OPENMP
    atomic_init(cb, val);
#else
    *cb = val;
#endif
}

bool ConcurrentBoolLoad(const ConcurrentBool *cb) {
#ifdef _OPENMP
    bool ret = atomic_load(cb);
#else
    bool ret = *cb;
#endif

    return ret;
}

void ConcurrentBoolStore(ConcurrentBool *cb, bool val) {
#ifdef _OPENMP
    atomic_store(cb, val);
#else
    *cb = val;
#endif
}

void ConcurrentIntInit(ConcurrentInt *ci, int val) {
#ifdef _OPENMP
    atomic_init(ci, val);
#else
    *ci = val;
#endif
}

int ConcurrentIntLoad(const ConcurrentInt *ci) {
#ifdef _OPENMP
    int ret = atomic_load(ci);
#else
    int ret = *ci;
#endif

    return ret;
}

void ConcurrentIntStore(ConcurrentInt *ci, int val) {
#ifdef _OPENMP
    atomic_store(ci, val);
#else
    *ci = val;
#endif
}

void ConcurrentSizeTypeInit(ConcurrentSizeType *cs, size_t val) {
#ifdef _OPENMP
    atomic_init(cs, val);
#else
    *cs = val;
#endif  // _OPENMP
}

size_t ConcurrentSizeTypeLoad(const ConcurrentSizeType *cs) {
#ifdef _OPENMP
    size_t ret = atomic_load(cs);
#else
    size_t ret = *cs;
#endif  // _OPENMP

    return ret;
}

size_t ConcurrentSizeTypeAdd(ConcurrentSizeType *cs, size_t val) {
#ifdef _OPENMP
    size_t ret = atomic_fetch_add(cs, val);
#else
    size_t ret = *cs;
    *cs += val;
#endif  // _OPENMP

    return ret;
}

size_t ConcurrentSizeTypeSubtract(ConcurrentSizeType *cs, size_t val) {
#ifdef _OPENMP
    size_t ret = atomic_fetch_sub(cs, val);
#else
    size_t ret = *cs;
    *cs -= val;
#endif  // _OPENMP

    return ret;
}

bool ConcurrentSizeTypeSubtractIfGreaterEqual(ConcurrentSizeType *cs,
                                              size_t val) {
#ifdef _OPENMP
    size_t cur_size = atomic_load(cs);
    while (cur_size >= val) {
        bool success =
            atomic_compare_exchange_weak(cs, &cur_size, cur_size - val);
        if (success) return true;
    }

    return false;
#else
    if (*cs < val) return false;
    *cs -= val;

    return true;
#endif  // _OPENMP
}

int ConcurrencyGetOmpNumThreads(void) {
#ifdef _OPENMP
    return omp_get_max_threads();
#else   // _OPENMP not defined.
    return 1;
#endif  // _OPENMP
}

int ConcurrencyGetOmpThreadId(void) {
#ifdef _OPENMP
    return omp_get_thread_num();
#else   // _OPENMP not defined, thread 0 is the only available thread.
    return 0;
#endif  // _OPENMP
}

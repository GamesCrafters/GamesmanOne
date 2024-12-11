/**
 * @file concurrency.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the concurrency convenience library.
 * @version 1.0.0
 * @date 2024-12-10
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

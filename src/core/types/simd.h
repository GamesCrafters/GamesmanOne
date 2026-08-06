/**
 * @file simd.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief SIMD helper library.
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

#ifndef GAMESMANONE_CORE_TYPES_SIMD_H_
#define GAMESMANONE_CORE_TYPES_SIMD_H_

#include <stdint.h>

typedef int8_t I8x16 __attribute__((vector_size(16)));
typedef uint8_t U8x16 __attribute__((vector_size(16)));
typedef uint64_t U64x2 __attribute__((vector_size(16)));

#include <stdint.h>

#ifdef GAMESMAN_HAS_BMI1
#include <immintrin.h>
#endif  // GAMESMAN_HAS_BMI1

static inline uint64_t PextU64(uint64_t val, uint64_t mask) {
#ifdef GAMESMAN_HAS_BMI2
    return _pext_u64(val, mask);
#else
    uint64_t res = 0;
    for (uint64_t bit = 1; mask != 0; bit <<= 1) {
        if (val & mask & -mask) {
            res |= bit;
        }
        mask &= mask - 1;
    }
    return res;
#endif
}

static inline uint64_t PdepU64(uint64_t val, uint64_t mask) {
#ifdef GAMESMAN_HAS_BMI2
    return _pdep_u64(val, mask);
#else
    uint64_t res = 0;
    for (uint64_t bit = 1; mask != 0; bit <<= 1) {
        if (val & bit) {
            res |= mask & -mask;
        }
        mask &= mask - 1;
    }
    return res;
#endif
}

static inline uint64_t BlsrU64(uint64_t x) {
#ifdef GAMESMAN_HAS_BMI1
    return _blsr_u64(x);
#else   // No BMI1
    return x & (x - 1);
#endif  // GAMESMAN_HAS_BMI1
}

static inline uint64_t BlsiU64(uint64_t x) {
#ifdef GAMESMAN_HAS_BMI1
    return _blsi_u64(x);
#else   // No BMI1
    return x & -x;
#endif  // GAMESMAN_HAS_BMI1
}

#endif  // GAMESMANONE_CORE_TYPES_SIMD_H_

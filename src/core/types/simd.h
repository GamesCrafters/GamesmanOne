#ifndef GAMESMANONE_CORE_TYPES_SIMD_H_
#define GAMESMANONE_CORE_TYPES_SIMD_H_

#include <stdint.h>

typedef int8_t I8x16 __attribute__((vector_size(16)));
typedef uint8_t U8x16 __attribute__((vector_size(16)));
typedef uint64_t U64x2 __attribute__((vector_size(16)));

#include <stdint.h>

#ifdef GAMESMAN_HAS_BMI2
#include <immintrin.h>
#endif

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

static inline uint64_t BlsrU64(uint64_t x) { return x & (x - 1); }

static inline uint64_t BlsiU64(uint64_t x) { return x & -x; }

#endif  // GAMESMANONE_CORE_TYPES_SIMD_H_

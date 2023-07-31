#include "gamesman_math.h"

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

bool IsPrime(int64_t n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int64_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
    }
    return true;
}

int64_t PrevPrime(int64_t n) {
    if (n < 2) return 2;
    while (!IsPrime(n)) {
        --n;
    }
    return n;
}

int64_t NextPrime(int64_t n) {
    while (!IsPrime(n)) {
        ++n;
    }
    return n;
}

int64_t SafeAddNonNegativeInt64(int64_t a, int64_t b) {
    if (a < 0 || b < 0 || a > INT64_MAX - b) return -1;
    return a + b;
}

int64_t SafeMultiplyNonNegativeInt64(int64_t a, int64_t b) {
    if (a < 0 || b < 0 || a > INT64_MAX / b) return -1;
    return a * b;
}

static int64_t NChooseRFormula(int n, int r) {
    assert(n >= 0 && r >= 0 && n >= r);

    // nCr(n, r) == nCr(n, n-r). This step can further reduce the largest
    // intermediate value.
    if (r > n - r) r = n - r;
    int64_t result = 1;
    for (int64_t i = 1; i <= r; ++i) {
        result = SafeMultiplyNonNegativeInt64(result, n - r + i);
        if (result < 0) return -1;
        result /= i;
    }
    return result;
}

#define CACHE_ROWS 100
#define CACHE_COLS 100
// Assumes CHOOSE has been zero initialized.
static void MakeTriangle(int64_t choose[][CACHE_COLS]) {
    for (int i = 0; i < CACHE_ROWS; ++i) {
        choose[i][0] = 1;
        int j_max = (i < CACHE_COLS - 1) ? i : CACHE_COLS - 1;
        for (int j = 1; j <= j_max; ++j) {
            choose[i][j] =
                SafeAddNonNegativeInt64(choose[i - 1][j - 1], choose[i - 1][j]);
        }
    }
}

int64_t NChooseR(int n, int r) {
    // Initialize cache.
    static bool choose_initialized = false;
    static int64_t choose[CACHE_ROWS][CACHE_COLS] = {0};

    if (!choose_initialized) {
        MakeTriangle(choose);
        choose_initialized = true;
    }

    if (n < 0 || r < 0) return -1;  // Negative inputs not supported.
    if (n < r) return 0;  // Make sure n >= r >= 0 in the following steps.
    if (n < CACHE_ROWS && r < CACHE_COLS) return choose[n][r];  // Cache hit.
    return NChooseRFormula(n, r);  // Cache miss. Calculate from formula.
}
#undef CACHE_ROWS
#undef CACHE_COLS

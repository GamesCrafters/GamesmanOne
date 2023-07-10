#include "misc.h"

#include <assert.h>  // assert
#include <malloc.h>  // free
#include <stdbool.h>  // bool, true, false
#include <stdint.h>  // int64_t

#include "int64_array.h"

inline void PositionArrayInit(PositionArray *array) { Int64ArrayInit(array); }

inline void PositionArrayDestroy(PositionArray *array) {
    Int64ArrayDestroy(array);
}

inline bool PositionArrayAppend(PositionArray *array, Position position) {
    return Int64ArrayPushBack(array, position);
}

inline bool PositionArrayContains(PositionArray *array, Position position) {
    return Int64ArrayContains(array, position);
}

void MoveListDestroy(MoveList list) {
    while (list != NULL) {
        MoveListItem *next = list->next;
        free(list);
        list = next;
    }
}

inline void TierArrayInit(TierArray *array) { Int64ArrayInit(array); }

inline void TierArrayDestroy(TierArray *array) { Int64ArrayDestroy(array); }

inline bool TierArrayAppend(TierArray *array, Tier tier) {
    return Int64ArrayPushBack(array, tier);
}

inline void TierStackInit(TierStack *stack) { Int64ArrayInit(stack); }

inline void TierStackDestroy(TierStack *stack) { Int64ArrayDestroy(stack); }

inline bool TierStackPush(TierStack *stack, Tier tier) {
    Int64ArrayPushBack(stack, tier);
}

inline void TierStackPop(TierStack *stack) { Int64ArrayPopBack(stack); }

inline Tier TierStackTop(TierStack *stack) { return Int64ArrayBack(stack); }

inline bool TierStackEmpty(TierStack *stack) { return Int64ArrayEmpty(stack); }

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
    while (!is_prime(n)) {
        --n;
    }
    return n;
}

int64_t NextPrime(int64_t n) {
    while (!is_prime(n)) {
        ++n;
    }
    return n;
}

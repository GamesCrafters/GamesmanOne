/**
 * @file int64_array_test.c
 * @brief Unit tests for the Int64Array module.
 */

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/data_structures/int64_array.h"

/* A custom comparison function for Int64ArraySortExplicit(). */
static int CompareDesc(const void *a, const void *b) {
    int64_t lhs = *(const int64_t *)a;
    int64_t rhs = *(const int64_t *)b;

    if (lhs < rhs)
        return 1; /* Return positive if lhs < rhs for descending order. */
    if (lhs > rhs) return -1;
    return 0;
}

static int TestInt64ArrayInit(void) {
    Int64Array array;
    Int64ArrayInit(&array);
    if (array.size != 0) return 1;
    Int64ArrayDestroy(&array);

    return 0;
}

static int TestInt64ArrayPushBack(void) {
    Int64Array array;
    Int64ArrayInit(&array);

    /* Push 5 elements and check. */
    if (!Int64ArrayPushBack(&array, 10)) return 1;
    if (!Int64ArrayPushBack(&array, 20)) return 1;
    if (!Int64ArrayPushBack(&array, 30)) return 1;
    if (!Int64ArrayPushBack(&array, 40)) return 1;
    if (!Int64ArrayPushBack(&array, 50)) return 1;
    if (array.size != 5) return 1;

    /* The back should be 50 now. */
    if (Int64ArrayBack(&array) != 50) return 1;

    /* Push more elements to ensure reallocation. */
    for (int64_t i = 0; i < 100; i++) {
        if (!Int64ArrayPushBack(&array, i)) return 1;
    }

    /* The array now has 105 elements (5 + 100). */
    if (array.size != 105) return 1;

    /* The last item should be 99, since we started pushing from 0 to 99. */
    if (Int64ArrayBack(&array) != 99) return 1;

    Int64ArrayDestroy(&array);

    return 0;
}

static int TestInt64ArrayPopBack(void) {
    Int64Array array;
    Int64ArrayInit(&array);

    /* Push some elements. */
    for (int64_t i = 0; i < 5; ++i) {
        Int64ArrayPushBack(&array, i);
    }
    if (array.size != 5) return 1;

    /* Pop them and check. */
    Int64ArrayPopBack(&array);
    if (array.size != 4) return 1;
    if (Int64ArrayBack(&array) != 3) return 1;

    Int64ArrayPopBack(&array);
    if (array.size != 3) return 1;
    if (Int64ArrayBack(&array) != 2) return 1;

    Int64ArrayPopBack(&array);
    Int64ArrayPopBack(&array);
    Int64ArrayPopBack(&array);
    /* Now array.size == 0. We do NOT test popping an empty array because
       that leads to undefined behavior as stated in the header. */

    if (array.size != 0) return 1;
    if (!Int64ArrayEmpty(&array)) return 1;

    Int64ArrayDestroy(&array);

    return 0;
}

static int TestInt64ArrayEmpty(void) {
    Int64Array array;
    Int64ArrayInit(&array);

    if (!Int64ArrayEmpty(&array)) return 1;
    Int64ArrayPushBack(&array, 100);
    if (Int64ArrayEmpty(&array)) return 1;
    Int64ArrayDestroy(&array);

    return 0;
}

static int TestInt64ArrayContains(void) {
    Int64Array array;
    Int64ArrayInit(&array);

    /* Populate array with known elements. */
    Int64ArrayPushBack(&array, 10);
    Int64ArrayPushBack(&array, -3);
    Int64ArrayPushBack(&array, 5);

    if (!Int64ArrayContains(&array, 10)) return 1;
    if (!Int64ArrayContains(&array, -3)) return 1;
    if (!Int64ArrayContains(&array, 5)) return 1;
    if (Int64ArrayContains(&array, 42)) return 1;

    Int64ArrayDestroy(&array);

    return 0;
}

static int TestInt64ArraySortAscending(void) {
    Int64Array array;
    Int64ArrayInit(&array);

    int64_t values[] = {5, -2, 10, 0, 3};
    for (int i = 0; i < 5; i++) {
        Int64ArrayPushBack(&array, values[i]);
    }

    Int64ArraySortAscending(&array);
    /* The sorted array (ascending) should be: -2, 0, 3, 5, 10 */
    if (array.size != 5) return 1;
    if (array.array[0] != -2) return 1;
    if (array.array[1] != 0) return 1;
    if (array.array[2] != 3) return 1;
    if (array.array[3] != 5) return 1;
    if (array.array[4] != 10) return 1;

    Int64ArrayDestroy(&array);

    return 0;
}

static int TestInt64ArraySortExplicit(void) {
    Int64Array array;
    Int64ArrayInit(&array);

    int64_t values[] = {5, -2, 10, 0, 3};
    for (int i = 0; i < 5; i++) {
        Int64ArrayPushBack(&array, values[i]);
    }

    /* Use a descending comparison. */
    Int64ArraySortExplicit(&array, CompareDesc);
    /* The sorted array (descending) should be: 10, 5, 3, 0, -2 */
    if (array.size != 5) return 1;
    if (array.array[0] != 10) return 1;
    if (array.array[1] != 5) return 1;
    if (array.array[2] != 3) return 1;
    if (array.array[3] != 0) return 1;
    if (array.array[4] != -2) return 1;

    Int64ArrayDestroy(&array);

    return 0;
}

static int TestInt64ArrayResize(void) {
    Int64Array array;
    Int64ArrayInit(&array);

    /* Resize up from 0 to 5. Should fill with zeros. */
    if (!Int64ArrayResize(&array, 5)) return 1;
    if (array.size != 5) return 1;
    for (int64_t i = 0; i < 5; i++) {
        if (array.array[i] != 0) return 1;
    }

    /* Push a couple of values. */
    Int64ArrayPushBack(&array, 42);
    Int64ArrayPushBack(&array, 99);
    if (array.size != 7) return 1;

    /* Resize down to 3; should lose the extra elements. */
    if (!Int64ArrayResize(&array, 3)) return 1;
    if (array.size != 3) return 1;

    Int64ArrayDestroy(&array);

    return 0;
}

static int TestInt64ArrayRemoveIndex(void) {
    Int64Array array;
    Int64ArrayInit(&array);

    /* Push elements: [0, 1, 2, 3, 4] */
    for (int64_t i = 0; i < 5; i++) {
        Int64ArrayPushBack(&array, i);
    }

    /* Remove index 2 -> removes the value 2; the array should now be [0, 1,
     * 3, 4]. */
    bool removed = Int64ArrayRemoveIndex(&array, 2);
    if (!removed) return 1;
    if (array.size != 4) return 1;
    if (array.array[0] != 0) return 1;
    if (array.array[1] != 1) return 1;
    if (array.array[2] != 3) return 1;
    if (array.array[3] != 4) return 1;

    /* Try removing out-of-range index. */
    removed = Int64ArrayRemoveIndex(&array, 10);
    if (removed) return 1;

    Int64ArrayDestroy(&array);

    return 0;
}

static int TestInt64ArrayRemove(void) {
    Int64Array array;
    Int64ArrayInit(&array);

    /* Push elements: [0, 1, 2, 3, 2, 4] */
    Int64ArrayPushBack(&array, 0);
    Int64ArrayPushBack(&array, 1);
    Int64ArrayPushBack(&array, 2);
    Int64ArrayPushBack(&array, 3);
    Int64ArrayPushBack(&array, 2);
    Int64ArrayPushBack(&array, 4);

    /* Remove first occurrence of 2 -> now [0, 1, 3, 2, 4]. */
    bool removed = Int64ArrayRemove(&array, 2);
    if (!removed) return 1;
    if (array.size != 5) return 1;
    if (array.array[0] != 0) return 1;
    if (array.array[1] != 1) return 1;
    if (array.array[2] != 3) return 1;
    if (array.array[3] != 2) return 1;
    if (array.array[4] != 4) return 1;

    /* Remove an element that doesn't exist. */
    removed = Int64ArrayRemove(&array, 999);
    if (removed) return 1;
    if (array.size != 5) return 1;

    Int64ArrayDestroy(&array);

    return 0;
}

static int TestInt64ArrayInitCopy(void) {
    Int64Array original;
    Int64ArrayInit(&original);

    for (int64_t i = 0; i < 5; i++) {
        Int64ArrayPushBack(&original, i * 2);  // [0, 2, 4, 6, 8]
    }

    Int64Array copy;
    if (!Int64ArrayInitCopy(&copy, &original)) return 1;
    if (copy.size != 5) return 1;
    if (copy.capacity < 5) return 1;

    /* Verify the contents match. */
    for (int64_t i = 0; i < 5; ++i) {
        if (copy.array[i] != original.array[i]) return 1;
    }

    /* Changing the original should not affect the copy. */
    Int64ArrayPushBack(&original, 10);
    if (original.size != 6) return 1;
    if (copy.size != 5) return 1;

    /* Cleanup. */
    Int64ArrayDestroy(&original);
    Int64ArrayDestroy(&copy);

    return 0;
}

int main(void) {
    if (TestInt64ArrayInit()) return EXIT_FAILURE;
    if (TestInt64ArrayPushBack()) return EXIT_FAILURE;
    if (TestInt64ArrayPopBack()) return EXIT_FAILURE;
    if (TestInt64ArrayEmpty()) return EXIT_FAILURE;
    if (TestInt64ArrayContains()) return EXIT_FAILURE;
    if (TestInt64ArraySortAscending()) return EXIT_FAILURE;
    if (TestInt64ArraySortExplicit()) return EXIT_FAILURE;
    if (TestInt64ArrayResize()) return EXIT_FAILURE;
    if (TestInt64ArrayRemoveIndex()) return EXIT_FAILURE;
    if (TestInt64ArrayRemove()) return EXIT_FAILURE;
    if (TestInt64ArrayInitCopy()) return EXIT_FAILURE;

    return 0;
}

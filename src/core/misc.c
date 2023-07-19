#include "core/misc.h"

#include <malloc.h>   // free
#include <stdbool.h>  // bool, true, false
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // exit

#include "core/data_structures/int64_array.h"
#include "core/data_structures/int64_hash_map.h"
#include "core/data_structures/int64_queue.h"

void NotReached(const char *message) {
    fprintf(stderr,
            "(FATAL) You entered a branch that is marked as NotReached. The "
            "error message was %s\n",
            message);
    exit(1);
}

void PositionArrayInit(PositionArray *array) { Int64ArrayInit(array); }

void PositionArrayDestroy(PositionArray *array) { Int64ArrayDestroy(array); }

bool PositionArrayAppend(PositionArray *array, Position position) {
    return Int64ArrayPushBack(array, position);
}

bool PositionArrayContains(PositionArray *array, Position position) {
    return Int64ArrayContains(array, position);
}

void PositionHashSetInit(PositionHashSet *set, double max_load_factor) {
    Int64HashMapInit(set, max_load_factor);
}

void PositionHashSetDestroy(PositionHashSet *set) {
    Int64HashMapDestroy(set);
}

bool PositionHashSetContains(PositionHashSet *set, Position position) {
    return Int64HashMapContains(set, position);
}

bool PositionHashSetAdd(PositionHashSet *set, Position position) {
    return Int64HashMapSet(set, position, 0);
}

void MoveArrayInit(MoveArray *array) { Int64ArrayInit(array); }

void MoveArrayDestroy(MoveArray *array) { Int64ArrayDestroy(array); }

bool MoveArrayAppend(MoveArray *array, Move move) {
    return Int64ArrayPushBack(array, move);
}

void TierArrayInit(TierArray *array) { Int64ArrayInit(array); }

void TierArrayDestroy(TierArray *array) { Int64ArrayDestroy(array); }

bool TierArrayAppend(TierArray *array, Tier tier) {
    return Int64ArrayPushBack(array, tier);
}

void TierStackInit(TierStack *stack) { Int64ArrayInit(stack); }

void TierStackDestroy(TierStack *stack) { Int64ArrayDestroy(stack); }

bool TierStackPush(TierStack *stack, Tier tier) {
    return Int64ArrayPushBack(stack, tier);
}

void TierStackPop(TierStack *stack) { Int64ArrayPopBack(stack); }

Tier TierStackTop(const TierStack *stack) { return Int64ArrayBack(stack); }

bool TierStackEmpty(const TierStack *stack) { return Int64ArrayEmpty(stack); }

void TierQueueInit(TierQueue *queue) { Int64QueueInit(queue); }

void TierQueueDestroy(TierQueue *queue) { Int64QueueDestroy(queue); }

bool TierQueueIsEmpty(const TierQueue *queue) {
    return Int64QueueIsEmpty(queue);
}

int64_t TierQueueSize(const TierQueue *queue) { return Int64QueueSize(queue); }

bool TierQueuePush(TierQueue *queue, Tier tier) {
    return Int64QueuePush(queue, tier);
}

Tier TierQueuePop(TierQueue *queue) { return Int64QueuePop(queue); }

void TierHashMapInit(TierHashMap *map, double max_load_factor) {
    Int64HashMapInit(map, max_load_factor);
}

void TierHashMapDestroy(TierHashMap *map) { Int64HashMapDestroy(map); }

TierHashMapIterator TierHashMapGet(TierHashMap *map, Tier key) {
    return Int64HashMapGet(map, key);
}

bool TierHashMapSet(TierHashMap *map, Tier tier, int64_t value) {
    return Int64HashMapSet(map, tier, value);
}

bool TierHashMapContains(TierHashMap *map, Tier tier) {
    return Int64HashMapContains(map, tier);
}

void TierHashMapIteratorInit(TierHashMapIterator *it, TierHashMap *map) {
    Int64HashMapIteratorInit(it, map);
}

Tier TierHashMapIteratorKey(const TierHashMapIterator *it) {
    return Int64HashMapIteratorKey(it);
}

int64_t TierHashMapIteratorValue(const TierHashMapIterator *it) {
    return Int64HashMapIteratorValue(it);
}

bool TierHashMapIteratorIsValid(const TierHashMapIterator *it) {
    return Int64HashMapIteratorIsValid(it);
}

bool TierHashMapIteratorNext(TierHashMapIterator *iterator, Tier *tier,
                             int64_t *value) {
    return Int64HashMapIteratorNext(iterator, tier, value);
}

void TierHashSetInit(TierHashSet *set, double max_load_factor) {
    Int64HashMapInit(set, max_load_factor);
}

void TierHashSetDestroy(TierHashSet *set) {
    Int64HashMapDestroy(set);
}

bool TierHashSetContains(TierHashSet *set, Tier tier) {
    return Int64HashMapContains(set, tier);
}

bool TierHashSetAdd(TierHashSet *set, Tier tier) {
    return Int64HashMapSet(set, tier, 0);
}

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

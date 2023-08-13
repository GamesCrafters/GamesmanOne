#include "core/misc.h"

#include <assert.h>     // assert
#include <errno.h>      // errno
#include <malloc.h>     // malloc, calloc, realloc, free
#include <math.h>       // INFINITY
#include <stdbool.h>    // bool, true, false
#include <stdint.h>     // int64_t
#include <stdio.h>      // fprintf, stderr
#include <stdlib.h>     // exit
#include <string.h>     // strlen, strncpy
#include <sys/stat.h>   // mkdir, struct stat
#include <sys/types.h>  // mode_t

#include "core/data_structures/int64_array.h"
#include "core/data_structures/int64_hash_map.h"
#include "core/data_structures/int64_queue.h"
#include "core/gamesman_math.h"

void NotReached(const char *message) {
    fprintf(stderr,
            "(FATAL) You entered a branch that is marked as NotReached. The "
            "error message was %s\n",
            message);
    exit(EXIT_FAILURE);
}

void *SafeMalloc(size_t size) {
    void *ret = malloc(size);
    if (ret == NULL) {
        fprintf(stderr,
                "SafeMalloc: failed to allocate %zd bytes. This ususally "
                "indicates a bug.\n",
                size);
        exit(EXIT_FAILURE);
    }
    return ret;
}

void *SafeCalloc(size_t n, size_t size) {
    void *ret = calloc(n, size);
    if (ret == NULL) {
        fprintf(stderr,
                "SafeCalloc: failed to allocate %zd elements each of %zd "
                "bytes. This ususally "
                "indicates a bug.\n",
                n, size);
        exit(EXIT_FAILURE);
    }
    return ret;
}

/**
 * @brief Makes a directory at the given path or does nothing if the directory
 * already exists.
 *
 * @return 0 on success. On error, -1 is returned and errno is set to indicate
 * the error.
 *
 * @author Jonathon Reinhart
 * @link
 * https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
 */
static int MaybeMkdir(const char *path, mode_t mode) {
    errno = 0;

    // Try to make the directory
    if (mkdir(path, mode) == 0) return 0;

    // If it fails for any reason but EEXIST, fail
    if (errno != EEXIST) return -1;

    // Check if the existing path is a directory
    struct stat st;
    if (stat(path, &st) != 0) return -1;

    // If not, fail with ENOTDIR
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }

    errno = 0;
    return 0;
}

int MkdirRecursive(const char *path) {
    // Fail if path is NULL.
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }

    int result = -1;
    errno = 0;

    // Copy string so it's mutable
    size_t path_length = strlen(path);
    char *path_copy = (char *)malloc((path_length + 1) * sizeof(char));
    if (path_copy == NULL) {
        errno = ENOMEM;
        goto _bailout;
    }
    strncpy(path_copy, path, path_length + 1);

    for (size_t i = 0; i < path_length; ++i) {
        if (path_copy[i] == '/') {
            path_copy[i] = '\0';  // Temporarily truncate
            if (MaybeMkdir(path_copy, 0777) != 0) goto _bailout;
            path_copy[i] = '/';
        }
    }
    if (MaybeMkdir(path_copy, 0777) != 0) goto _bailout;
    result = 0;  // Success

_bailout:
    free(path_copy);
    return result;
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

void PositionHashSetDestroy(PositionHashSet *set) { Int64HashMapDestroy(set); }

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

bool MoveArrayPopBack(MoveArray *array) {
    if (array->size <= 0) return false;
    --array->size;
    return true;
}

bool MoveArrayContains(const MoveArray *array, Move move) {
    return Int64ArrayContains(array, move);
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

void TierHashSetDestroy(TierHashSet *set) { Int64HashMapDestroy(set); }

bool TierHashSetContains(TierHashSet *set, Tier tier) {
    return Int64HashMapContains(set, tier);
}

bool TierHashSetAdd(TierHashSet *set, Tier tier) {
    return Int64HashMapSet(set, tier, 0);
}

void TierPositionArrayInit(TierPositionArray *array) {
    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
}

void TierPositionArrayDestroy(TierPositionArray *array) {
    free(array->array);
    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
}

static bool TierPositionArrayExpand(TierPositionArray *array) {
    int64_t new_capacity = array->capacity == 0 ? 1 : array->capacity * 2;
    TierPosition *new_array = (TierPosition *)realloc(
        array->array, new_capacity * sizeof(TierPosition));
    if (!new_array) return false;
    array->array = new_array;
    array->capacity = new_capacity;
    return true;
}

bool TierPositionArrayAppend(TierPositionArray *array,
                             TierPosition tier_position) {
    if (array->size == array->capacity) {
        if (!TierPositionArrayExpand(array)) return false;
    }
    assert(array->size < array->capacity);
    array->array[array->size++] = tier_position;
    return true;
}

TierPosition TierPositionArrayBack(const TierPositionArray *array) {
    return array->array[array->size - 1];
}

void TierPositionHashSetInit(TierPositionHashSet *set, double max_load_factor) {
    set->entries = NULL;
    set->capacity = 0;
    set->size = 0;
    if (max_load_factor > 0.75) max_load_factor = 0.75;
    if (max_load_factor < 0.25) max_load_factor = 0.25;
    set->max_load_factor = max_load_factor;
}

void TierPositionHashSetDestroy(TierPositionHashSet *set) {
    free(set->entries);
    set->entries = NULL;
    set->capacity = 0;
    set->size = 0;
}

static int64_t TierPositionHashSetHash(TierPosition key, int64_t capacity) {
    int64_t a = (int64_t)key.tier;
    int64_t b = (int64_t)key.position;
    int64_t cantor_pairing = (a + b) * (a + b + 1) / 2 + a;
    return ((uint64_t)cantor_pairing) % capacity;
}

bool TierPositionHashSetContains(TierPositionHashSet *set, TierPosition key) {
    int64_t capacity = set->capacity;
    // Edge case: return false if set is empty.
    if (set->capacity == 0) return false;
    int64_t index = TierPositionHashSetHash(key, capacity);
    while (set->entries[index].used) {
        TierPosition this_key = set->entries[index].key;
        if (this_key.tier == key.tier && this_key.position == key.position) {
            return true;
        }
        index = (index + 1) % capacity;
    }
    return false;
}

static bool TierPositionHashSetExpand(TierPositionHashSet *set) {
    int64_t new_capacity = NextPrime(set->capacity * 2);
    TierPositionHashSetEntry *new_entries = (TierPositionHashSetEntry *)calloc(
        new_capacity, sizeof(TierPositionHashSetEntry));
    if (new_entries == NULL) return false;
    for (int64_t i = 0; i < set->capacity; ++i) {
        if (set->entries[i].used) {
            int64_t new_index =
                TierPositionHashSetHash(set->entries[i].key, new_capacity);
            while (new_entries[new_index].used) {
                new_index = (new_index + 1) % new_capacity;
            }
            new_entries[new_index] = set->entries[i];
        }
    }
    free(set->entries);
    set->entries = new_entries;
    set->capacity = new_capacity;
    return true;
}

bool TierPositionHashSetAdd(TierPositionHashSet *set, TierPosition key) {
    // Check if resizing is needed.
    double load_factor;
    if (set->capacity == 0) {
        load_factor = INFINITY;
    } else {
        load_factor = (double)(set->size + 1) / (double)set->capacity;
    }
    if (load_factor > set->max_load_factor) {
        if (!TierPositionHashSetExpand(set)) return false;
    }

    // Set value at key.
    int64_t capacity = set->capacity;
    int64_t index = TierPositionHashSetHash(key, capacity);
    while (set->entries[index].used) {
        TierPosition this_key = set->entries[index].key;
        if (this_key.tier == key.tier && this_key.position == key.position) {
            return true;
        }
        index = (index + 1) % capacity;
    }
    set->entries[index].key = key;
    set->entries[index].used = true;
    ++set->size;
    return true;
}

int GameVariantToIndex(const GameVariant *variant) {
    int ret = 0;
    for (int i = 0; variant->options[i].num_choices > 0; ++i) {
        ret = ret * variant->options[i].num_choices + variant->selections[i];
    }
    return ret;
}

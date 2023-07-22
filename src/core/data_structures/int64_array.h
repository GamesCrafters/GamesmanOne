#ifndef GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_INT64_ARRAY_H_
#define GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_INT64_ARRAY_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

typedef struct Int64Array {
    int64_t *array;
    int64_t size;
    int64_t capacity;
} Int64Array;

void Int64ArrayInit(Int64Array *array);
void Int64ArrayDestroy(Int64Array *array);
bool Int64ArrayPushBack(Int64Array *array, int64_t item);
void Int64ArrayPopBack(Int64Array *array);
int64_t Int64ArrayBack(const Int64Array *array);
bool Int64ArrayEmpty(const Int64Array *array);
bool Int64ArrayContains(const Int64Array *array, int64_t item);

#endif  // GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_INT64_ARRAY_H_

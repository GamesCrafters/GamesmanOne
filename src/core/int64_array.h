#ifndef GAMESMANEXPERIMENT_CORE_INT64_ARRAY_H_
#define GAMESMANEXPERIMENT_CORE_INT64_ARRAY_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int64_t *array;
    int64_t size;
    int64_t capacity;
} Int64Array;

void Int64ArrayInit(Int64Array *array);
void Int64ArrayDestroy(Int64Array *array);
bool Int64ArrayPushBack(Int64Array *array, int64_t item);
void Int64ArrayPopBack(Int64Array *array);
int64_t Int64ArrayBack(Int64Array *array);
bool Int64ArrayEmpty(Int64Array *array);
bool Int64ArrayContains(Int64Array *array, int64_t item);

#endif  // GAMESMANEXPERIMENT_CORE_INT64_ARRAY_H_

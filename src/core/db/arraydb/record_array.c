#include "core/db/arraydb/record_array.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdint.h>  // int64_t
#include <stdlib.h>  // calloc, free

#include "core/db/arraydb/record.h"

int RecordArrayInit(RecordArray *array, int64_t size) {
    array->records = (Record *)calloc(size, sizeof(Record));
    if (array->records == NULL) return kMallocFailureError;
    array->size = size;

    return kNoError;
}

void RecordArrayDestroy(RecordArray *array) {
    free(array->records);
    array->records = NULL;
    array->size = 0;
}

void RecordArraySetValue(RecordArray *array, Position position, Value val) {
    assert(position >= 0 && position < array->size);
    RecordSetValue(&array->records[position], val);
}

void RecordArraySetRemoteness(RecordArray *array, Position position,
                              int remoteness) {
    assert(position >= 0 && position < array->size);
    RecordSetRemoteness(&array->records[position], remoteness);
}

Value RecordArrayGetValue(const RecordArray *array, Position position) {
    return RecordGetValue(&array->records[position]);
}

int RecordArrayGetRemoteness(const RecordArray *array, Position position) {
    return RecordGetRemoteness(&array->records[position]);
}

const void *RecordArrayGetReadOnlyData(const RecordArray *array) {
    return (const void *)array->records;
}

void *RecordArrayGetData(RecordArray *array) {
    return (void *)array->records;
}

const int64_t RecordArrayGetSize(const RecordArray *array) {
    return array->size;
}

const int64_t RecordArrayGetRawSize(const RecordArray *array) {
    return array->size * sizeof(Record);
}

#ifndef GAMESMANONE_CORE_DB_BPDB_RECORD_ARRAY_H_
#define GAMESMANONE_CORE_DB_BPDB_RECORD_ARRAY_H_

#include <stdint.h>  // int64_t

#include "core/db/arraydb/record.h"

typedef struct RecordArray {
    Record *records;
    int64_t size;
} RecordArray;

int RecordArrayInit(RecordArray *array, int64_t size);
void RecordArrayDestroy(RecordArray *array);

void RecordArraySetValue(RecordArray *array, Position position, Value val);
void RecordArraySetRemoteness(RecordArray *array, Position position,
                              int remoteness);

Value RecordArrayGetValue(const RecordArray *array, Position position);
int RecordArrayGetRemoteness(const RecordArray *array, Position position);

const void *RecordArrayGetReadOnlyData(const RecordArray *array);
void *RecordArrayGetData(RecordArray *array);
const int64_t RecordArrayGetSize(const RecordArray *array);
const int64_t RecordArrayGetRawSize(const RecordArray *array);

#endif  // GAMESMANONE_CORE_DB_BPDB_RECORD_ARRAY_H_

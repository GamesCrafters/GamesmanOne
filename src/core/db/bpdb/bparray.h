#ifndef GAMESMANEXPERIMENT_CORE_DB_BPDB_BPARRAY_H_
#define GAMESMANEXPERIMENT_CORE_DB_BPDB_BPARRAY_H_

#include <stdint.h>  // int8_t, uint8_t, int32_t, int64_t, uint64_t

#include "core/db/bpdb/lookup_dict.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#endif

typedef struct BpArrayMeta {
    int64_t stream_length;
    int64_t num_entries;
    int8_t bits_per_entry;
} BpArrayMeta;

// Bit-Perfect array
typedef struct BpArray {
    uint8_t *stream;
    LookupDict lookup;
    BpArrayMeta meta;
} BpArray;

int BpArrayInit(BpArray *array, int64_t size);
void BpArrayDestroy(BpArray *array);

// Returns the entry at index I.
uint64_t BpArrayGet(BpArray *array, int64_t i);

// Sets the entry at index I to ENTRY.
int BpArraySet(BpArray *array, int64_t i, uint64_t entry);

int32_t BpArrayGetNumUniqueValues(const BpArray *array);
const int32_t *BpArrayGetDecompDict(const BpArray *array);

#endif  // GAMESMANEXPERIMENT_CORE_DB_BPDB_BPARRAY_H_

#ifndef GAMESMANEXPERIMENT_CORE_DB_BPDB_BPARRAY_H_
#define GAMESMANEXPERIMENT_CORE_DB_BPDB_BPARRAY_H_

#include <stdint.h>  // int8_t, uint8_t, int64_t, uint64_t

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
    uint64_t max_value;
    BpArrayMeta meta;
} BpArray;

int BpArrayInit(BpArray *array, int64_t size);
void BpArrayDestroy(BpArray *array);

// Returns the entry at index I.
uint64_t BpArrayAt(const BpArray *array, int64_t i);

// Sets the I-th entry to ENTRY.
void BpArraySet(BpArray *array, int64_t i, uint64_t entry);

// Thread-safe version of BpArraySet().
int BpArraySetTs(BpArray *array, int64_t i, uint64_t entry);

#endif  // GAMESMANEXPERIMENT_CORE_DB_BPDB_BPARRAY_H_

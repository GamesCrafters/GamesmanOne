#ifndef GAMESMANEXPERIMENT_CORE_DB_BPDB_BPARRAY_H_
#define GAMESMANEXPERIMENT_CORE_DB_BPDB_BPARRAY_H_

#include <stdint.h>  // uint8_t, int64_t, uint64_t

typedef struct DecompDictMetadata {
    /**
     * Size of the decomp dictionary (including this variable) in bytes.
     */
    int32_t size;
} DecompDictMetadata;

typedef struct BitStreamMetadata {
    int64_t stream_length;
    int64_t num_entries;
    int bits_per_entry;
} BitStreamMetadata;

typedef struct LookupTableMetadata {
    /** Size of the lookup table (including this variable) in bytes. */
    int32_t size;
    int64_t block_size;
} LookupTableMetadata;

typedef struct BpdbFileHeader {
    DecompDictMetadata decomp_dict_metadata;
    BitStreamMetadata stream_metadata;
    LookupTableMetadata lookup_table_metadata;
} BpdbFileHeader;

// Byte-perfect array. The entries are originally uint64_t values but are
// compressed to use just enough bytes to represent all possible values.
typedef struct BpArray {
    uint8_t *array;
    int64_t num_entries;
    uint64_t max_value;
    int bytes_per_entry;
} BpArray;

typedef struct BitStream {
    uint8_t *stream;
    BitStreamMetadata metadata;
} BitStream;

int BpArrayInit(BpArray *array, int64_t size, uint64_t max_value);
void BpArrayDestroy(BpArray *array);

uint64_t BpArrayGet(const BpArray *array, int64_t index);
void BpArraySet(BpArray *array, int64_t index, uint64_t value);

int BpArrayCompress(const BpArray *array, BitStream *dest,
                    int32_t **decomp_dict, int32_t *decomp_dict_size);

void BitStreamDestroy(BitStream *stream);

#endif  // GAMESMANEXPERIMENT_CORE_DB_BPDB_BPARRAY_H_

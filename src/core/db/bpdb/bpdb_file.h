#ifndef GAMESMANEXPERIMENT_CORE_DB_BPDB_BPDB_FILE_H_
#define GAMESMANEXPERIMENT_CORE_DB_BPDB_BPDB_FILE_H_

#include <stdint.h>  // int64_t, int32_t

#include "core/db/bpdb/bparray.h"
#include "core/gamesman_types.h"

typedef struct DecompDictMeta {
    /** Size of the decomp dictionary in bytes. */
    int32_t size;
} DecompDictMeta;

typedef struct LookupTableMeta {
    /** Size of each mgz compression block. */
    int64_t block_size;

    /** Size of the lookup table in bytes. */
    int64_t size;
} LookupTableMeta;

typedef struct BpdbFileHeader {
    DecompDictMeta decomp_dict_meta;
    LookupTableMeta lookup_meta;
    BpArrayMeta stream_meta;
} BpdbFileHeader;

char *BpdbFileGetFullPath(ConstantReadOnlyString current_path, Tier tier);
int BpdbFileFlush(ReadOnlyString full_path, const BpArray *records,
                  const int32_t *decomp_dict, int32_t num_unique_values);
int BpdbFileGetBlockSize(int bits_per_entry);

#endif  // GAMESMANEXPERIMENT_CORE_DB_BPDB_BPDB_FILE_H_

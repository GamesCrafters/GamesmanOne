#ifndef GAMESMANONE_CORE_DB_BPDB_ARRAYDB_H_
#define GAMESMANONE_CORE_DB_BPDB_ARRAYDB_H_

#include "core/types/gamesman_types.h"

extern const Database kArrayDb;

typedef struct ArrayDbOptions {
    int block_size;
    int compression_level;
    int extreme_compression;
} ArrayDbOptions;
extern const ArrayDbOptions kArrayDbOptionsInit;

extern const int kArrayDbRecordSize;

#endif  // GAMESMANONE_CORE_DB_BPDB_ARRAYDB_H_

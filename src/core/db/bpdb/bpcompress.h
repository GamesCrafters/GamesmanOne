#ifndef GAMESMANEXPERIMENT_CORE_DB_BPDB_BPCOMPRESS_H_
#define GAMESMANEXPERIMENT_CORE_DB_BPDB_BPCOMPRESS_H_

#include <stdint.h>  // int32_t

#include "core/db/bpdb/bparray.h"

int BpCompress(BpArray *dest, const BpArray *src, int32_t **decomp_dict,
               int32_t *decomp_dict_size);

#endif  // GAMESMANEXPERIMENT_CORE_DB_BPDB_BPCOMPRESS_H_
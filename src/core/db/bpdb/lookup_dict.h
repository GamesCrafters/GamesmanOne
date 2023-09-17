#ifndef GAMESMANEXPERIMENT_CORE_DB_BPDB_LOOKUP_DICT_H_
#define GAMESMANEXPERIMENT_CORE_DB_BPDB_LOOKUP_DICT_H_

#include <stdint.h>  // int32_t

typedef struct LookupDict {
    int32_t *comp_dict;
    int32_t *decomp_dict;
    int32_t comp_dict_size;
    int32_t decomp_dict_capacity;
    int32_t num_unique;  // "decomp_dict_size"
} LookupDict;

int LookupDictInit(LookupDict *dict);
void LookupDictDestroy(LookupDict *dict);

int LookupDictSet(LookupDict *dict, int32_t key);
int32_t LookupDictGet(const LookupDict *dict, int32_t key);
int32_t LookupDictGetKey(const LookupDict *dict, int32_t value);

#endif  // GAMESMANEXPERIMENT_CORE_DB_BPDB_LOOKUP_DICT_H_

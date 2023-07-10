#ifndef GAMESMANEXPERIMENT_CORE_TIER_HASH_TABLE_H_
#define GAMESMANEXPERIMENT_CORE_TIER_HASH_TABLE_H_
#include <stdbool.h>
#include <stdint.h>

#include "gamesman_types.h"
#include "int64_hash_map.h"

typedef struct {
    int32_t num_unsolved_children;
    int8_t status;
} TierHashTableEntryExtra;
typedef Int64HashMapEntry TierHashTableEntry;
typedef Int64HashMap TierHashTable;
typedef Int64HashMapIterator TierHashTableIterator;
typedef TierHashTableEntry *TierHashTableEntryList;

bool TierHashTableInitialize(TierHashTable *table);
void TierHashTableDestroy(TierHashTable *table);
TierHashTableEntry *TierHashTableFind(TierHashTable *table, Tier tier);
void TierHashTableAdd(Tier tier);
TierHashTableEntry *TierHashTableDetach(TierHashTable *table, Tier tier);
void TierHashTableClear(TierHashTable *table);

bool TierHashTableIteratorInit(TierHashTableIterator *it, TierHashTable *table);
Tier TierHashTableIteratorGetNext(TierHashTableIterator *it);
void TierHashTableIteratorDestroy(TierHashTableIterator *it);

#endif  // GAMESMANEXPERIMENT_CORE_TIER_HASH_TABLE_H_

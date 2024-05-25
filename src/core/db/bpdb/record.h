#ifndef GAMESMANONE_CORE_DB_BPDB_RECORD_H_
#define GAMESMANONE_CORE_DB_BPDB_RECORD_H_

#include <stdint.h>  // uint16_t

#include "core/types/gamesman_types.h"

typedef uint16_t Record;

void RecordSetValue(Record *rec, Value val);
void RecordSetRemoteness(Record *rec, int remoteness);

Value RecordGetValue(const Record *rec);
int RecordGetRemoteness(const Record *rec);

#endif  // GAMESMANONE_CORE_DB_BPDB_RECORD_H_

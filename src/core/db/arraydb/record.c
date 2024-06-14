#include "core/db/arraydb/record.h"

#include <assert.h>  // assert
#include <stdint.h>  // uint16_t

#include "core/constants.h"
#include "core/types/gamesman_types.h"

static const int kRecordBits = sizeof(Record) * kBitsPerByte;
static const int kValueBits = 4;
static const int kRemotenessBits = kRecordBits - kValueBits;
static const uint16_t kRemotenessMask = (1 << kRemotenessBits) - 1;

void RecordSetValue(Record *rec, Value val) {
    assert(val >= 0 && val < (1 << kValueBits));
    uint16_t remoteness = RecordGetRemoteness(rec);
    *rec = (val << kRemotenessBits) | remoteness;
}

void RecordSetRemoteness(Record *rec, int remoteness) {
    assert(remoteness >= 0 && remoteness < (1 << kRemotenessBits));
    Value val = RecordGetValue(rec);
    *rec = (val << kRemotenessBits) | (uint16_t)remoteness;
}

Value RecordGetValue(const Record *rec) { return (*rec) >> kRemotenessBits; }

int RecordGetRemoteness(const Record *rec) { return (*rec) & kRemotenessMask; }

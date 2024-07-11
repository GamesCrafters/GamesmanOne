/**
 * @file record.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the basic record type for the Array Database, which
 * only stores values and remotenesses.
 * @version 1.0.0
 * @date 2024-07-10
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

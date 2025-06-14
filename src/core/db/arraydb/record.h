/**
 * @file record.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The basic record type for the Array Database, which only stores values
 * and remotenesses.
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

#ifndef GAMESMANONE_CORE_DB_BPDB_RECORD_H_
#define GAMESMANONE_CORE_DB_BPDB_RECORD_H_

#include <assert.h>  // assert
#include <stdint.h>  // uint16_t

#include "core/constants.h"
#include "core/types/gamesman_types.h"

/** @brief The record type. */
typedef uint16_t Record;

/** @brief Number of bits per Record. */
static const int kRecordBitsPerRecord = sizeof(Record) * kBitsPerByte;

/** @brief Number of bits for the value field in a Record. */
static const int kRecordNumValueBits = 4;

/** @brief Number of bits for the remoteness field in a Record. */
static const int kRecordNumRemotenessBits =
    kRecordBitsPerRecord - kRecordNumValueBits;

/**
 * @brief Returns the value field of record \p rec.
 *
 * @param rec Source record.
 * @return Value field of \p rec.
 */
static inline Value RecordGetValue(const Record *rec) {
    return (*rec) >> kRecordNumRemotenessBits;
}

/**
 * @brief Returns the remoteness field of record \p rec.
 *
 * @param rec Source record.
 * @return Remoteness field of \p rec.
 */
static inline int RecordGetRemoteness(const Record *rec) {
    static const uint16_t remoteness_mask =
        (1U << kRecordNumRemotenessBits) - 1;

    return (*rec) & remoteness_mask;
}

/**
 * @brief Sets the value field of record \p rec to \p val.
 *
 * @param rec Target record.
 * @param val New value.
 */
static inline void RecordSetValue(Record *rec, Value val) {
    assert(val >= 0 && val < (1 << kRecordNumValueBits));
    uint16_t remoteness = RecordGetRemoteness(rec);
    *rec = (val << kRecordNumRemotenessBits) | remoteness;
}

/**
 * @brief Sets the remoteness field of record \p rec to \p remoteness.
 *
 * @param rec Target record.
 * @param remoteness New remoteness.
 */
static inline void RecordSetRemoteness(Record *rec, int remoteness) {
    assert(remoteness >= 0 && remoteness < (1 << kRecordNumRemotenessBits));
    Value val = RecordGetValue(rec);
    *rec = (val << kRecordNumRemotenessBits) | (uint16_t)remoteness;
}

/**
 * @brief Sets the value and remoteness fields of record \p rec to \p val and
 * \p remoteness , respectively.
 *
 * @param rec Target record.
 * @param val New value.
 * @param remoteness New remoteness.
 */
static inline void RecordSetValueRemoteness(Record *rec, Value val,
                                            int remoteness) {
    assert(val >= 0 && val < (1 << kRecordNumValueBits));
    assert(remoteness >= 0 && remoteness < (1 << kRecordNumRemotenessBits));
    *rec = (val << kRecordNumRemotenessBits) | remoteness;
}

/**
 * @brief Replaces the value and remoteness fields of record \p rec with the
 * maximum of its original value-remoteness pair and the one provided by \p val
 * and \p remoteness . The order of value-remoteness pairs are determined by the
 * \p compare function.
 *
 * @param rec Target record.
 * @param val Candidate value.
 * @param remoteness Candidate remoteness.
 * @param compare Pointer to a value-remoteness pair comparison function that
 * takes in two value-remoteness pairs (v1, r1) and (v2, r2) and returns a
 * negative integer if (v1, r1) < (v2, r2), a positive integer if (v1, r1) >
 * (v2, r2), or zero if they are equal.
 */
static inline void RecordMaximize(Record *rec, Value val, int remoteness,
                                  int (*compare)(Value v1, int r1, Value v2,
                                                 int r2)) {
    Value old_val = RecordGetValue(rec);
    int old_rmt = RecordGetRemoteness(rec);
    if (compare(old_val, old_rmt, val, remoteness) < 0) {
        RecordSetValueRemoteness(rec, val, remoteness);
    }
}

#endif  // GAMESMANONE_CORE_DB_BPDB_RECORD_H_

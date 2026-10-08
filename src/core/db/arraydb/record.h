/**
 * @file record.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The basic record type for the Array Database, which only stores values
 * and remotenesses.
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

#ifndef GAMESMANONE_CORE_DB_ARRAYDB_RECORD_H_
#define GAMESMANONE_CORE_DB_ARRAYDB_RECORD_H_

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "core/constants.h"

/** The `Record` type. */
typedef uint16_t Record;

/** Constants. */
enum {
    /** Number of bits per `Record`. */
    kRecordBitsPerRecord = sizeof(Record) * kBitsPerByte,

    /** Number of bits for the value field in a `Record`. */
    kRecordNumValueBits = 4,

    /** Number of bits for the remoteness field in a `Record`. */
    kRecordNumRemotenessBits = kRecordBitsPerRecord - kRecordNumValueBits,
};

/**
 * @brief Returns the value field of `Record` `rec`.
 *
 * @param[in] rec Source `Record`.
 *
 * @returns Value field of `rec`.
 */
static inline Value RecordGetValue(const Record *rec) {
    return (*rec) >> kRecordNumRemotenessBits;
}

/**
 * @brief Returns the remoteness field of `Record` `rec`.
 *
 * @param[in] rec Source `Record`.
 *
 * @returns Remoteness field of `rec`.
 */
static inline int RecordGetRemoteness(const Record *rec) {
    const uint16_t remoteness_mask = (1U << kRecordNumRemotenessBits) - 1;

    return (*rec) & remoteness_mask;
}

/**
 * @brief Returns the number of undecided children of `Record` `rec`. Returns 0
 * if its value is not `kUndecided`.
 *
 * @note The remoteness field of a `Record` is overloaded as the counter for the
 * number of undecided children of that position when its value is `kUndecided`.
 *
 * @param[in] rec Source `Record`.
 *
 * @returns The number of undecided children of `Record` `rec`, or 0 if its
 * value is not `kUndecided`.
 */
static inline int RecordGetNumUndecidedChildren(const Record *rec) {
    if (RecordGetValue(rec) != kUndecided) {
        return 0;
    }

    return RecordGetRemoteness(rec);
}

/**
 * @brief Sets the value field of `Record` `rec` to `val`.
 *
 * @param[in,out] rec Target `Record`.
 * @param[in] val New `Value`.
 */
static inline void RecordSetValue(Record *rec, Value val) {
    assert(val >= 0 && val < (1 << kRecordNumValueBits));  // LCOV_EXCL_BR_LINE

    uint16_t remoteness = RecordGetRemoteness(rec);
    *rec = (val << kRecordNumRemotenessBits) | remoteness;
}

/**
 * @brief Sets the remoteness field of `Record` `rec` to `remoteness`.
 *
 * @param[in,out] rec Target `Record`.
 * @param[in] remoteness New remoteness.
 */
static inline void RecordSetRemoteness(Record *rec, int remoteness) {
    // LCOV_EXCL_BR_START
    assert(remoteness >= 0 && remoteness < (1 << kRecordNumRemotenessBits));
    // LCOV_EXCL_BR_STOP

    Value val = RecordGetValue(rec);
    *rec = (val << kRecordNumRemotenessBits) | (uint16_t)remoteness;
}

/**
 * @brief Sets the value and remoteness fields of `Record` `rec` to `val` and
 * `remoteness`, respectively.
 *
 * @param[in,out] rec Target `Record`.
 * @param[in] val New `Value`.
 * @param[in] remoteness New remoteness.
 */
static inline void RecordSetValueRemoteness(Record *rec, Value val,
                                            int remoteness) {
    // LCOV_EXCL_BR_START
    assert(val >= 0 && val < (1 << kRecordNumValueBits));
    assert(remoteness >= 0 && remoteness < (1 << kRecordNumRemotenessBits));
    // LCOV_EXCL_BR_STOP

    *rec = (val << kRecordNumRemotenessBits) | remoteness;
}

/**
 * @brief Replaces the value and remoteness fields of `Record` `rec` with the
 * maximum of its original value-remoteness pair and the one provided by `val`
 * and `remoteness`. The order of value-remoteness pairs is determined by the
 * `compare` function.
 *
 * @param[in,out] rec Target `Record`.
 * @param[in] val Candidate `Value`.
 * @param[in] remoteness Candidate remoteness.
 * @param[in] compare Pointer to a value-remoteness pair comparison function
 * that takes in two value-remoteness pairs (v1, r1) and (v2, r2) and returns a
 * negative integer if (v1, r1) < (v2, r2), a positive integer if (v1, r1) >
 * (v2, r2), or zero if they are equal.
 *
 * @retval true If the provided `val`-`remoteness` pair is greater than
 * the original value-remoteness pair and the old pair is replaced.
 * @retval false Otherwise.
 */
static inline bool RecordMaximize(Record *rec, Value val, int remoteness,
                                  int (*compare)(Value v1, int r1, Value v2,
                                                 int r2)) {
    Value old_val = RecordGetValue(rec);
    int old_rmt = RecordGetRemoteness(rec);
    if (compare(old_val, old_rmt, val, remoteness) < 0) {
        RecordSetValueRemoteness(rec, val, remoteness);
        return true;
    }

    return false;
}

/**
 * @brief Subtracts one from the number of undecided children of `Record` `rec`
 * and returns the value immediately preceding the subtraction if the value
 * field of `rec` is set to `kUndecided` and the number of undecided children
 * is at least one. Does nothing and returns 0 otherwise.
 *
 * @note The remoteness field of a `Record` is overloaded as the counter for the
 * number of undecided children of that position when its value is `kUndecided`.
 *
 * @param[in,out] rec Target `Record`.
 *
 * @returns Number of undecided children immediately preceding the subtraction,
 * or 0 if the subtraction is not performed.
 */
static inline int RecordDecrementNumUndecidedChildren(Record *rec) {
    if (RecordGetValue(rec) != kUndecided) {
        return 0;
    }

    int num_undecided = RecordGetRemoteness(rec);
    if (num_undecided == 0) {
        return 0;
    }

    RecordSetRemoteness(rec, num_undecided - 1);

    return num_undecided;
}

/**
 * @brief Sets the number of undecided children of `Record` `rec` to zero and
 * returns the value immediately preceding the operation if the value field of
 * `rec` is set to `kUndecided`. Does nothing and returns 0 otherwise.
 *
 * @note The remoteness field of a `Record` is overloaded as the counter for the
 * number of undecided children of that position when its value is `kUndecided`.
 *
 * @param[in,out] rec Target `Record`.
 *
 * @returns Number of undecided children immediately preceding the operation, or
 * 0 if the operation is not performed.
 */
static inline int RecordClearNumUndecidedChildren(Record *rec) {
    if (RecordGetValue(rec) != kUndecided) {
        return 0;
    }

    int num_undecided = RecordGetRemoteness(rec);
    if (num_undecided == 0) {
        return 0;
    }

    RecordSetRemoteness(rec, 0);

    return num_undecided;
}

#endif  // GAMESMANONE_CORE_DB_ARRAYDB_RECORD_H_

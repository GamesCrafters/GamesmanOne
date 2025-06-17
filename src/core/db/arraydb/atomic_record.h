/**
 * @file atomic_record.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The atomic version of the basic record type for the Array Database
 * storing only values and remotenesses.
 * @version 1.0.0
 * @date 2025-06-09
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

#ifndef GAMESMANONE_CORE_DB_BPDB_ATOMIC_RECORD_H_
#define GAMESMANONE_CORE_DB_BPDB_ATOMIC_RECORD_H_

#include <stdatomic.h>  //

#include "core/db/arraydb/record.h"
#include "core/types/gamesman_types.h"

/** @brief The atomic record type. */
typedef _Atomic Record AtomicRecord;

/**
 * @brief Initializes record \p ar with the given \p value and \p remoteness .
 *
 * @param ar Pointer to the AtmoicRecord to initialize.
 * @param value Initial value.
 * @param remoteness Initial remoteness.
 */
static inline void AtmoicRecordInit(AtomicRecord *ar, Value value,
                                    int remoteness) {
    Record rec;
    RecordSetValueRemoteness(&rec, value, remoteness);
    atomic_init(ar, rec);
}

/**
 * @brief Returns the value field of record \p ar .
 *
 * @param ar Source record.
 * @return Value field of \p ar.
 */
static inline Value AtomicRecordGetValue(const AtomicRecord *ar) {
    Record rec = atomic_load_explicit(ar, memory_order_relaxed);

    return RecordGetValue(&rec);
}

/**
 * @brief Returns the remoteness field of record \p ar.
 *
 * @param ar Source record.
 * @return Remoteness field of \p ar.
 */
static inline int AtomicRecordGetRemoteness(const AtomicRecord *ar) {
    Record rec = atomic_load_explicit(ar, memory_order_relaxed);

    return RecordGetRemoteness(&rec);
}

/**
 * @brief Atomically sets the value field of record \p ar to \p val .
 *
 * @param ar Target record.
 * @param val New value.
 */
static inline void AtomicRecordSetValue(AtomicRecord *ar, Value val) {
    Record old_rec = atomic_load_explicit(ar, memory_order_relaxed);
    while (1) {
        Record new_rec = old_rec;
        RecordSetValue(&new_rec, val);
        if (atomic_compare_exchange_weak_explicit(ar, &old_rec, new_rec,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            return;
        }
    }
}

/**
 * @brief Atomically sets the remoteness field of record \p ar to \p remoteness
 * .
 *
 * @param ar Target record.
 * @param remoteness New remoteness.
 */
static inline void AtomicRecordSetRemoteness(AtomicRecord *ar, int remoteness) {
    Record old_rec = atomic_load_explicit(ar, memory_order_relaxed);
    while (1) {
        Record new_rec = old_rec;
        RecordSetRemoteness(&new_rec, remoteness);
        if (atomic_compare_exchange_weak_explicit(ar, &old_rec, new_rec,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            return;
        }
    }
}

/**
 * @brief Atomically sets the value and remoteness fields of record \p ar to
 * \p val and \p remoteness , respectively.
 *
 * @param ar Target record.
 * @param val New value.
 * @param remoteness New remoteness.
 */
static inline void AtomicRecordSetValueRemoteness(AtomicRecord *ar, Value val,
                                                  int remoteness) {
    Record rec;
    RecordSetValueRemoteness(&rec, val, remoteness);
    atomic_store_explicit(ar, rec, memory_order_relaxed);
}

/**
 * @brief Atomically replaces the value and remoteness fields of record \p ar
 * with the maximum of its original value-remoteness pair and the one provided
 * by \p val and \p remoteness . The order of value-remoteness pairs are
 * determined by the \p compare function.
 *
 * @param ar Target record.
 * @param val Candidate value.
 * @param remoteness Candidate remoteness.
 * @param compare Pointer to a value-remoteness pair comparison function that
 * takes in two value-remoteness pairs (v1, r1) and (v2, r2) and returns a
 * negative integer if (v1, r1) < (v2, r2), a positive integer if (v1, r1) >
 * (v2, r2), or zero if they are equal.
 * @return \c true if the provided \p value - \p remoteness pair is greater than
 * the original value-remoteness pair and the old pair is replaced;
 * @return \c false otherwise.
 */
static inline bool AtomicRecordMaximize(AtomicRecord *ar, Value val,
                                        int remoteness,
                                        int (*compare)(Value v1, int r1,
                                                       Value v2, int r2)) {
    Record old_rec = atomic_load_explicit(ar, memory_order_relaxed);
    Value old_val = RecordGetValue(&old_rec);
    int old_rmt = RecordGetRemoteness(&old_rec);
    if (compare(old_val, old_rmt, val, remoteness) >= 0) return false;

    Record new_rec;
    RecordSetValueRemoteness(&new_rec, val, remoteness);
    do {
        if (atomic_compare_exchange_weak_explicit(ar, &old_rec, new_rec,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            return true;
        }
        old_val = RecordGetValue(&old_rec);
        old_rmt = RecordGetRemoteness(&old_rec);
    } while (compare(old_val, old_rmt, val, remoteness) < 0);

    return false;
}

static inline Record AtomicRecordLoad(const AtomicRecord *ar) {
    return atomic_load_explicit(ar, memory_order_relaxed);
}

#endif  // GAMESMANONE_CORE_DB_BPDB_ATOMIC_RECORD_H_

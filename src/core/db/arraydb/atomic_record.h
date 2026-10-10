/**
 * @file atomic_record.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The atomic version of the basic `Record` type for the Array Database
 * storing only values and remotenesses.
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

#ifndef GAMESMANONE_CORE_DB_ARRAYDB_ATOMIC_RECORD_H_
#define GAMESMANONE_CORE_DB_ARRAYDB_ATOMIC_RECORD_H_

#include <stdatomic.h>

#include "core/db/arraydb/record.h"

/** The atomic `Record` type. */
typedef _Atomic Record AtomicRecord;

/**
 * @brief Initializes `ar` with the given `value` and `remoteness`.
 *
 * @param[out] ar Pointer to the `AtomicRecord` to initialize.
 * @param[in] value Initial `Value`.
 * @param[in] remoteness Initial remoteness.
 */
static inline void AtmoicRecordInit(AtomicRecord *ar, Value value,
                                    int remoteness) {
    Record rec;
    RecordSetValueRemoteness(&rec, value, remoteness);
    atomic_init(ar, rec);
}

/**
 * @brief Retrieves the raw `Record` value from `ar`.
 *
 * @param[in] ar Source `AtomicRecord`.
 *
 * @return Raw `Record` value stored in `ar`.
 */
static inline Record AtomicRecordLoad(const AtomicRecord *ar) {
    return atomic_load_explicit(ar, memory_order_relaxed);
}

/**
 * @brief Returns the value field of `ar`.
 *
 * @param[in] ar Source `AtomicRecord`.
 *
 * @return Value field of `ar`.
 */
static inline Value AtomicRecordGetValue(const AtomicRecord *ar) {
    Record rec = AtomicRecordLoad(ar);

    return RecordGetValue(&rec);
}

/**
 * @brief Returns the remoteness field of `ar`.
 *
 * @param[in] ar Source `AtomicRecord`.
 *
 * @return Remoteness field of `ar`.
 */
static inline int AtomicRecordGetRemoteness(const AtomicRecord *ar) {
    Record rec = AtomicRecordLoad(ar);

    return RecordGetRemoteness(&rec);
}

/**
 * @brief Returns the number of undecided children of `ar`. Returns 0 if its
 * value is not `kUndecided`.
 *
 * @note The remoteness field of a `Record` is overloaded as the counter for the
 * number of undecided children of that position when its value is `kUndecided`.
 *
 * @param[in] ar Source `AtomicRecord`.
 *
 * @return The number of undecided children of `ar`, or 0 if its value is
 * not `kUndecided`.
 */
static inline int AtomicRecordGetNumUndecidedChildren(const AtomicRecord *ar) {
    Record rec = AtomicRecordLoad(ar);

    return RecordGetNumUndecidedChildren(&rec);
}

/**
 * @brief Atomically sets the value field of `ar` to `val`.
 *
 * @param[in,out] ar Target `AtomicRecord`.
 * @param[in] val New `Value`.
 */
static inline void AtomicRecordSetValue(AtomicRecord *ar, Value val) {
    Record old_rec = AtomicRecordLoad(ar);
    for (;;) {
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
 * @brief Atomically sets the remoteness field of `ar` to `remoteness`.
 *
 * @param[in,out] ar Target `AtomicRecord`.
 * @param[in] remoteness New remoteness.
 */
static inline void AtomicRecordSetRemoteness(AtomicRecord *ar, int remoteness) {
    Record old_rec = AtomicRecordLoad(ar);
    for (;;) {
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
 * @brief Atomically sets the value and remoteness fields of `ar` to
 * `val` and `remoteness`, respectively.
 *
 * @param[in,out] ar Target `AtomicRecord`.
 * @param[in] val New `Value`.
 * @param[in] remoteness New remoteness.
 */
static inline void AtomicRecordSetValueRemoteness(AtomicRecord *ar, Value val,
                                                  int remoteness) {
    Record rec;
    RecordSetValueRemoteness(&rec, val, remoteness);
    atomic_store_explicit(ar, rec, memory_order_relaxed);
}

/**
 * @brief Atomically replaces the value and remoteness fields of `ar`
 * with the maximum of its original value-remoteness pair and the one provided
 * by `val` and `remoteness`. The order of value-remoteness pairs is
 * determined by the `compare` function.
 *
 * @param[in,out] ar Target `AtomicRecord`.
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
static inline bool AtomicRecordMaximize(AtomicRecord *ar, Value val,
                                        int remoteness,
                                        int (*compare)(Value v1, int r1,
                                                       Value v2, int r2)) {
    Record old_rec = AtomicRecordLoad(ar);
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

/**
 * @brief Atomically subtracts one from the number of undecided children of
 * `ar` and returns the number of undecided children immediately preceding the
 * subtraction if the value field of `ar` is set to `kUndecided` and the number
 * of undecided children is at least one. Does nothing and returns 0 if the
 * value field of `ar` is not `kUndecided` or the number of undecided children
 * is zero.
 *
 * @note The remoteness field of a `Record` is overloaded as the counter for the
 * number of undecided children of that position when its value is `kUndecided`.
 *
 * @param[in,out] ar Target `AtomicRecord`.
 *
 * @return Number of undecided children immediately preceding the subtraction,
 * or 0 if the subtraction is not performed.
 */
static inline int AtomicRecordDecrementNumUndecidedChildren(AtomicRecord *ar) {
    Record old_rec = AtomicRecordLoad(ar);
    int num_undecided;
    while ((num_undecided = RecordGetNumUndecidedChildren(&old_rec)) > 0) {
        Record new_rec = old_rec;
        RecordDecrementNumUndecidedChildren(&new_rec);
        if (atomic_compare_exchange_weak_explicit(ar, &old_rec, new_rec,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            break;
        }
    }

    return num_undecided;
}

/**
 * @brief Atomically sets the number of undecided children of `ar` to
 * zero and returns the number of undecided children immediately preceding the
 * operation if the value field of `ar` is set to `kUndecided`. Does nothing
 * and returns 0 if the value field of `ar` is not `kUndecided`.
 *
 * @note The remoteness field of a `Record` is overloaded as the counter for the
 * number of undecided children of that position when its value is `kUndecided`.
 *
 * @param[in,out] ar Target `AtomicRecord`.
 *
 * @return Number of undecided children immediately preceding the operation, or
 * 0 if the operation is not performed.
 */
static inline int AtomicRecordClearNumUndecidedChildren(AtomicRecord *ar) {
    Record old_rec = AtomicRecordLoad(ar);
    int num_undecided;
    while ((num_undecided = RecordGetNumUndecidedChildren(&old_rec)) > 0) {
        Record new_rec = old_rec;
        RecordClearNumUndecidedChildren(&new_rec);
        if (atomic_compare_exchange_weak_explicit(ar, &old_rec, new_rec,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            break;
        }
    }

    return num_undecided;
}

#endif  // GAMESMANONE_CORE_DB_ARRAYDB_ATOMIC_RECORD_H_

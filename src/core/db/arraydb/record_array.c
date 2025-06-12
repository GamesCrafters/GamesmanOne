/**
 * @file record_array.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the fixed-length \c Record array for the Array
 * Database.
 * @version 1.0.1
 * @date 2024-12-22
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

#include "core/db/arraydb/record_array.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL, size_t
#include <stdint.h>  // int64_t
#include <string.h>  // memset

#include "core/db/arraydb/record.h"
#include "core/gamesman_memory.h"

struct RecordArray {
    int64_t size;
    Record records[];
};

RecordArray *RecordArrayCreate(int64_t size) {
    RecordArray *ret =
        (RecordArray *)GamesmanMalloc(sizeof(int64_t) + size * sizeof(Record));
    if (ret == NULL) return NULL;

    ret->size = size;
    memset(ret->records, 0, size * sizeof(Record));

    return ret;
}

void RecordArrayDestroy(RecordArray *array) { GamesmanFree(array); }

void RecordArraySetValue(RecordArray *array, Position position, Value val) {
    assert(position >= 0 && position < array->size);
    RecordSetValue(&array->records[position], val);
}

void RecordArraySetRemoteness(RecordArray *array, Position position,
                              int remoteness) {
    assert(position >= 0 && position < array->size);
    RecordSetRemoteness(&array->records[position], remoteness);
}

void RecordArraySetValueRemoteness(RecordArray *array, Position position,
                                   Value val, int remoteness) {
    assert(position >= 0 && position < array->size);
    RecordSetValueRemoteness(&array->records[position], val, remoteness);
}

Value RecordArrayGetValue(const RecordArray *array, Position position) {
    return RecordGetValue(&array->records[position]);
}

int RecordArrayGetRemoteness(const RecordArray *array, Position position) {
    return RecordGetRemoteness(&array->records[position]);
}

const void *RecordArrayGetReadOnlyData(const RecordArray *array) {
    return (const void *)array->records;
}

void *RecordArrayGetData(RecordArray *array) { return (void *)array->records; }

int64_t RecordArrayGetSize(const RecordArray *array) { return array->size; }

size_t RecordArrayGetRawSize(const RecordArray *array) {
    return array->size * sizeof(Record);
}

/**
 * @file db_probe.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Generic `Database` probe type.
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

#ifndef GAMESMANONE_CORE_TYPES_DATABASE_DB_PROBE_H_
#define GAMESMANONE_CORE_TYPES_DATABASE_DB_PROBE_H_

#include <stdint.h>

#include "core/types/base.h"

/**
 * @brief A `Database` probe used to access a database on permanent storage
 * (disk). To access an in-memory database, use the `Database` "Solving API"
 * instead.
 *
 * @details A `DbProbe` is an abstract type for a database probe. It is used
 * to extract information, such as the value and remoteness, from a database. It
 * is a helper type for the `Database` type, which uses this structure to cache
 * the most recently used data from an on-disk database. A `DbProbe` can be
 * implemented to read ahead and cache the most recently used database contents
 * in its internal buffer. Its exact behavior depends on the implementation of
 * the corresponding `Database`.
 */
typedef struct DbProbe {
    /** The most recently used tier. */
    Tier tier;

    /** The read-ahead buffer. */
    void *buffer;

    /**
     * Stores the starting index of the buffer. However, a database
     * implementation may override this definition.
     */
    int64_t begin;

    /**
     * Stores the size of the buffer. However, a database implementation may
     * override this definition.
     */
    int64_t size;
} DbProbe;

#endif  // GAMESMANONE_CORE_TYPES_DATABASE_DB_PROBE_H_

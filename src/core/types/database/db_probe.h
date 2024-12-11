/**
 * @file db_probe.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Generic Database probe type.
 * @details A DbProbe is an abstract type for a database probe, which is used to
 * extract information such as value and remoteness from a database. It is
 * considered a helper type for the Database type, which uses this structure to
 * cache most recently used data from on-disk database. The behavior depends on
 * the implementation of the corresponding Databse.
 * @version 1.0.1
 * @date 2024-12-10
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

#include <stdint.h>  // int64_t

#include "core/types/base.h"

/**
 * @brief Database probe which can be used to probe the database on permanent
 * storage (disk). To access in-memory DB, use Database's "Solving API" instead.
 *
 * @details A DbProbe can be implemented to read ahead and cache most recently
 * used database contents in its internal buffer. The behavior is implementation
 * dependent.
 */
typedef struct DbProbe {
    Tier tier;    /**< Most recently used tier. */
    void *buffer; /**< Read-ahead buffer. */

    /** Stores the index to the begin position of the buffer. However, the
     * database implementation may override this definition. */
    int64_t begin;

    /** Stores the size of the buffer. However, the database implementation may
     * override this definition. */
    int64_t size;
} DbProbe;

#endif  // GAMESMANONE_CORE_TYPES_DATABASE_DB_PROBE_H_

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

#include <stdint.h>  // uint16_t

#include "core/types/gamesman_types.h"

/** @brief The record type. */
typedef uint16_t Record;

/**
 * @brief Sets the value field of record \p rec to \p val.
 * 
 * @param rec Target record.
 * @param val New value.
 */
void RecordSetValue(Record *rec, Value val);

/**
 * @brief Sets the remoteness field of record \p rec to \p remoteness.
 * 
 * @param rec Target record.
 * @param remoteness New remoteness.
 */
void RecordSetRemoteness(Record *rec, int remoteness);

/**
 * @brief Returns the value field of record \p rec.
 * 
 * @param rec Source record.
 * @return Value field of \p rec.
 */
Value RecordGetValue(const Record *rec);

/**
 * @brief Returns the remoteness field of record \p rec.
 * 
 * @param rec Source record.
 * @return Remoteness field of \p rec.
 */
int RecordGetRemoteness(const Record *rec);

#endif  // GAMESMANONE_CORE_DB_BPDB_RECORD_H_

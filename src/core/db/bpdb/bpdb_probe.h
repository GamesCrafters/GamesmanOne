/**
 * @file bpdb_probe.h
 * @author Dan Garcia: designed the "lookup table" compression algorithm
 * @author Max Fierro: improved the algorithm for BpArray compression
 * @author Sameer Nayyar: improved the algorithm for BpArray compression
 * @author Robert Shi (robertyishi@berkeley.edu): improved and implemented
 * BpDict.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Probe for the Bit-Perfect Database.
 * @version 1.1.1
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

#ifndef GAMESMANONE_CORE_DB_BPDB_BPDB_PROBE_H_
#define GAMESMANONE_CORE_DB_BPDB_BPDB_PROBE_H_

#include <stdint.h>  // uint64_t

#include "core/types/gamesman_types.h"

/**
 * @brief Initializes PROBE.
 *
 * @return 0 on success, or
 * @return kMallocFailureError on malloc failure.
 */
int BpdbProbeInit(DbProbe *probe);

/**
 * @brief Destroys PROBE and frees allocated memory.
 *
 * @return 0 always.
 */
int BpdbProbeDestroy(DbProbe *probe);

/**
 * @brief Probes the record for TIER_POSITION using the given PROBE and returns
 * it.
 *
 * @param sandbox_path Path to the sandbox directory provided to BPDB.
 * @param probe Initialized database probe to use.
 * @param tier_position Tier position to query.
 * @param GetTierName Function that converts a tier to its name. If set to
 * NULL, a fallback method will be used instead.
 * @return Record encoded as an unsigned integer.
 */
uint64_t BpdbProbeRecord(ConstantReadOnlyString sandbox_path, DbProbe *probe,
                         TierPosition tier_position,
                         GetTierNameFunc GetTierName);

#endif  // GAMESMANONE_CORE_DB_BPDB_BPDB_PROBE_H_

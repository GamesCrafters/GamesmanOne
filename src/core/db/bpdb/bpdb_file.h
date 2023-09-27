/**
 * @file bpdb_file.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Bit-Perfect Database file utilities.
 * @version 1.0
 * @date 2023-09-26
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

#ifndef GAMESMANEXPERIMENT_CORE_DB_BPDB_BPDB_FILE_H_
#define GAMESMANEXPERIMENT_CORE_DB_BPDB_BPDB_FILE_H_

#include <stdint.h>  // int64_t, int32_t

#include "core/db/bpdb/bparray.h"
#include "core/gamesman_types.h"

/** @brief Decompression dictionary metadata. */
typedef struct DecompDictMeta {
    /** Size of the decomp dictionary in bytes. */
    int32_t size;
} DecompDictMeta;

/** @brief MGZ lookup table metadata. */
typedef struct LookupTableMeta {
    /** Size of each MGZ compression block. */
    int64_t block_size;

    /** Size of the lookup table in bytes. */
    int64_t size;
} LookupTableMeta;

/** @brief In-memory bpdb file header. */
typedef struct BpdbFileHeader {
    DecompDictMeta decomp_dict_meta;
    LookupTableMeta lookup_meta;
    BpArrayMeta stream_meta;
} BpdbFileHeader;

/**
 * @brief Returns the full path to the bpdb file for the given TIER given the
 * SANDBOX_PATH for bpdb. The user is responsible for free()ing the pointer
 * returned by this function.
 *
 * @param sandbox_path Path to a sandbox directory for bpdb.
 * @param tier Get path to the db file of this TIER.
 * @return Full path to the bpdb file on success, or
 * @return NULL on failure.
 */
char *BpdbFileGetFullPath(ConstantReadOnlyString sandbox_path, Tier tier);

/**
 * @brief Flushes the RECORDS to a bpdb file under FULL_PATH.
 *
 * @param full_path Full path to the bpdb file to create.
 * @param records Solver records encoded as a BpArray.
 * @return 0 on success, or
 * @return non-zero error code on failure.
 */
int BpdbFileFlush(ReadOnlyString full_path, const BpArray *records);

/**
 * @brief Returns the proper MGZ block size (in bytes) to use given the number
 * of bits used to store each entry in the BpArray.
 *
 * @details MGZ divides the bit stream into blocks of block_size bytes,
 * compresses all blocks in parallel, and then concatenates them. To allow
 * random access to the database file, a constant number of blocks will be
 * loaded from disk and buffered in a DbProbe obejct. To speed up sequential
 * access of the entire db file, a proper block size should be chosen so that
 * each block contains a whole number of entries to avoid repeated loading of
 * the same block when an entry lies on the boundary of two adjacent blocks.
 *
 * @param bits_per_entry Number of bits used to store each entry.
 * @return Block size in bytes.
 */
int BpdbFileGetBlockSize(int bits_per_entry);

#endif  // GAMESMANEXPERIMENT_CORE_DB_BPDB_BPDB_FILE_H_

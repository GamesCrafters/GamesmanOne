/**
 * @file bpdb_file.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Bit-Perfect Database file utilities.
 * @version 1.2.2
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

#ifndef GAMESMANONE_CORE_DB_BPDB_BPDB_FILE_H_
#define GAMESMANONE_CORE_DB_BPDB_BPDB_FILE_H_

#include <stdint.h>  // int64_t, int32_t

#include "core/db/bpdb/bparray.h"
#include "core/types/gamesman_types.h"

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
    /** Metadata for the decompression dictionary. */
    DecompDictMeta decomp_dict_meta;

    /** Metadata for the block compression lookup table. */
    LookupTableMeta lookup_meta;

    /** Metadata for the data stream. */
    BpArrayMeta stream_meta;
} BpdbFileHeader;

/**
 * @brief Returns a malloc'ed full path to the bpdb file for the given \p tier
 * given \p sandbox_path for bpdb and an optional function which converts
 * \p tier to its file name. The user is responsible for freeing the pointer
 * returned by this function using the \c free function.
 *
 * @param sandbox_path Path to a sandbox directory for bpdb.
 * @param tier Get path to the db file of this \p tier.
 * @param GetTierName Function that converts a tier to its name. If set to
 * \c NULL, a fallback method will be used instead.
 * @return Full path to the bpdb file on success, or
 * @return \c NULL on failure.
 */
char *BpdbFileGetFullPath(ConstantReadOnlyString sandbox_path, Tier tier,
                          GetTierNameFunc GetTierName);

/**
 * @brief Returns a malloc'ed full path to the finish flag for the current game
 * given \p sandbox_path. The user is responsible for freeing the pointer
 * returned by this function using the \c free function.
 *
 * @param sandbox_path Path to a sandbox directory for bpdb.
 * @return Full path to the the finish flag on success, or
 * @return \c NULL on failure.
 */
char *BpdbFileGetFullPathToFinishFlag(ConstantReadOnlyString sandbox_path);

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
 * the same block when some entries are stored across the boundary of two
 * adjacent blocks.
 *
 * @param bits_per_entry Number of bits used to store each entry.
 * @return Block size in bytes.
 */
int64_t BpdbFileGetBlockSize(int bits_per_entry);

/**
 * @brief Returns the status of TIER stored under the given SANDBOX_PATH for
 * bpdb.
 *
 * @param sandbox_path Path to a sandbox directory for bpdb.
 * @param tier The tier to get status of.
 * @param GetTierName Function that converts a tier to its name. If set to
 * NULL, a fallback method will be used instead.
 * @return kDbTierStatusSolved if solved,
 * @return kDbTierStatusCorrupted if corrupted,
 * @return kDbTierStatusMissing if not solved, or
 * @return kDbTierStatusCheckError if an error occurred when checking the status
 * of TIER.
 */
int BpdbFileGetTierStatus(ConstantReadOnlyString sandbox_path, Tier tier,
                          GetTierNameFunc GetTierName);

#endif  // GAMESMANONE_CORE_DB_BPDB_BPDB_FILE_H_

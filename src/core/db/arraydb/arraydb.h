/**
 * @file arraydb.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Simple array database which stores value-remoteness pairs in a 16-bit
 * record array.
 * @details The in-memory database is an uncompressed 16-bit record array of
 * length equal to the size of the given tier. The array is block-compressed
 * using LZMA provided by the XZ Utils library wrapped in the XZRA (XZ with
 * random access) library.
 * @version 1.0.1
 * @date 2024-08-25
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

#ifndef GAMESMANONE_CORE_DB_ARRAYDB_ARRAYDB_H_
#define GAMESMANONE_CORE_DB_ARRAYDB_ARRAYDB_H_

#include "core/types/gamesman_types.h"

/**
 * @brief Simple array database which stores value-remoteness pairs in a 16-bit
 * record array.
 */
extern const Database kArrayDb;

/**
 * @brief ArrayDb options. Pass a pointer to an instance of this type to the
 * initialization function (kArrayDb::Init) to use custom settings, or pass
 * \c NULL for default options.
 */
typedef struct ArrayDbOptions {
    /** Size of each LZMA compression block in bytes. Larger blocks provides
     * better compression ratio at the cost of increased random-access delay.
     * Default: 1048576 (1 MiB). */
    int block_size;

    /** LZMA compression level. Ranges from 0 (store) to 9 (ultra). Using levels
     * 7-9 may increase memory usage. Default: 6. */
    int compression_level;

    /** Set this to 1 to enable extreme LZMA compression, which slightly
     * improves compression ratio at the cost of significantly increased
     * (typically doubled) compression time. */
    int extreme_compression;
} ArrayDbOptions;

/**
 * @brief Default \c ArrayDbOptions for convenient initialization of
 * ArrayDbOptions instances.
 */
extern const ArrayDbOptions kArrayDbOptionsInit;

/**
 * @brief Size of each ArrayDb record.
 */
extern const int kArrayDbRecordSize;

#endif  // GAMESMANONE_CORE_DB_ARRAYDB_ARRAYDB_H_

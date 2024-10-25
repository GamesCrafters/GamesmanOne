/**
 * @file bpdict.h
 * @author Dan Garcia: designed the "lookup table" compression algorithm
 * @author Max Fierro: improved the algorithm for BpArray compression
 * @author Sameer Nayyar: improved the algorithm for BpArray compression
 * @author Robert Shi (robertyishi@berkeley.edu): improved and implemented
 * BpDict.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Compression and decompression dictionaries for Bit-Perfect Array.
 * @version 1.0.1
 * @date 2024-10-16
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

#ifndef GAMESMANONE_CORE_DB_BPDB_BPDICT_H_
#define GAMESMANONE_CORE_DB_BPDB_BPDICT_H_

#include <stdint.h>  // int32_t

/** @brief Dictionaries for BpArray compression and decompresion.*/
typedef struct BpDict {
    /** Compression dictionary as an entry-indexed array. Maps unique entries in
     * the array to their encoded values. */
    int32_t *comp_dict;

    /** Decompresion dictionary as an encoded-value-indexed array. Maps encoded
     * values to unique entries in the array. */
    int32_t *decomp_dict;

    /** Size of the compression dictionary in number of entries. */
    int32_t comp_dict_size;

    /** Capacity of the decompression dictionary in number of entries. */
    int32_t decomp_dict_capacity;

    /** Number of unique entries mapped so far. Equals the number of valid
     * entry-value pairs in the decompression dictionary. */
    int32_t num_unique;
} BpDict;

/**
 * @brief Initializes the given DICT to contain exactly one entry 0 mapped to
 * the encoded value 0.
 *
 * @details The current implementation of BpArray always initializes the empty
 * solver array to all zeros. A zero entry is defined as invalid as it
 * corresponds to the undecided position value.
 *
 * @note Assumes DICT has not been initialized. May result in memory leak and/or
 * other undefined behaviors otherwise.
 */
int BpDictInit(BpDict *dict);

/** @brief Destroys the given DICT. */
void BpDictDestroy(BpDict *dict);

/**
 * @brief Add a new encoding in DICT for the given KEY, assuming KEY does not
 * already exist in DICT.
 *
 * @note The user of this function is responsible for checking the existance
 * of KEY using the BpDictGet() method before calling this function.
 *
 * @param dict Dictionary in use.
 * @param key New entry to add as key.
 * @return 0 on success, or
 * @return non-zero error code otherwise.
 */
int BpDictSet(BpDict *dict, int32_t key);

/**
 * @brief Returns the encoded value corresponds to KEY in DICT, or -1 if KEY
 * does not exist in DICT.
 * @details This function is used as part of the compression algorithm.
 */
int32_t BpDictGet(const BpDict *dict, int32_t key);

/**
 * @brief Returns the entry corresponds to the given encoded VALUE from DICT.
 *
 * @note Assumes VALUE is valid in DICT. Results in undefined behavior
 * otherwise.
 */
int32_t BpDictGetKey(const BpDict *dict, int32_t value);

#endif  // GAMESMANONE_CORE_DB_BPDB_BPDICT_H_

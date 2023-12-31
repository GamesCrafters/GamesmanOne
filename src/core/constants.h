/**
 * @file constants.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Global constants.
 * @version 1.0
 * @date 2023-10-21
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

#include "core/gamesman_types.h"

/**
 * @brief Precalculated string length limits for integers of different types.
 */
enum IntBase10StringLengthLimits {
    /** int8_t: [-128, 127] */
    kInt8Base10StringLengthMax = 4,

    /** uint8_t: [0, 255] */
    kUint8Base10StringLengthMax = 3,

    /** int16_t: [-32768, 32767] */
    kInt16Base10StringLengthMax = 6,

    /** uint16_t: [0, 65535] */
    kUint16Base10StringLengthMax = 5,

    /** int32_t: [-2147483648, 2147483647] */
    kInt32Base10StringLengthMax = 11,

    /** uint32_t: [0, 4294967295] */
    kUint32Base10StringLengthMax = 10,

    /** int64_t: [-9223372036854775808, 9223372036854775807] */
    kInt64Base10StringLengthMax = 20,

    /** uint64_t: [0, 18446744073709551615] */
    kUint64Base10StringLengthMax = 20,
};

enum CommonConstants {
    kIllegalSize = -1, /**< Valid sizes are non-negative. */
    kBitsPerByte = 8,  /**< 8 bits per byte. */
};

extern const Tier kIllegalTier;
extern const Position kIllegalPosition;
extern const TierPosition kIllegalTierPosition;
extern const int kIllegalRemoteness;

extern ConstantReadOnlyString kDate;
extern ConstantReadOnlyString kVersion;

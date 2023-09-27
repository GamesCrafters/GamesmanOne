/**
 * @file gz64.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of 64-bit gzip utilities.
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

#include "libs/mgz/gz64.h"

#define GZ_READ_CHUNK_SIZE INT_MAX

/* Wrapper function around gzread using 64-bit unsigned integer
   as read size and 64-bit signed integer as return type to
   allow the reading of more than INT_MAX bytes. */
int64_t gz64_read(gzFile file, voidp buf, uint64_t len) {
    int64_t total = 0;
    int bytesRead = 0;

    /* Read INT_MAX bytes at a time. */
    while (len > (uint64_t)GZ_READ_CHUNK_SIZE) {
        bytesRead = gzread(file, buf, GZ_READ_CHUNK_SIZE);
        if (bytesRead != GZ_READ_CHUNK_SIZE) return (int64_t)bytesRead;

        total += bytesRead;
        len -= bytesRead;
        buf = (voidp)((uint8_t*)buf + bytesRead);
    }

    /* Read the rest. */
    bytesRead = gzread(file, buf, (unsigned int)len);
    if (bytesRead != (int)len) return (int64_t)bytesRead;

    return total + bytesRead;
}

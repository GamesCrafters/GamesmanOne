/**
 * @file mgz.h
 * @author Cameron Cheung: designed the first algorithm that allows random
 * access to gzip archives.
 * @author Robert Shi (robertyishi@berkeley.edu): OpenMP parallelization and
 * implementation of MGZ.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief In-Memory Gzip.
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

#ifndef GAMESMANEXPERIMENT_LIBS_MGZ_MGZ_H_
#define GAMESMANEXPERIMENT_LIBS_MGZ_MGZ_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    void *out;
    int64_t size;
    int64_t *lookup;
    int64_t num_blocks;
} mgz_res_t;

/**
 * @brief Compresses INSIZE bytes of data from IN using compression
 * level LEVEL and stores the compressed data in a malloc()ed array at
 * *OUT.
 *
 * @details Assumes OUT points to a valid (void *). Sets *OUT to NULL and
 * returns 0 if INSIZE is 0. Sets *OUT to NULL and returns a negative error code
 * if an error occurs. The user is responsible for free()ing the output buffer
 * at *OUT.
 *
 * @param out Pointer to a valid generic pointer (void *) which will be set to a
 * malloc'ed array storing the compressed bytes on success.
 * @param in Input.
 * @param in_size Size of the input in bytes.
 * @param level Gzip compression level which can be any integer from -1 to 9. -1
 * gives zlib's default compression level, 0 gives no compression, 1 gives best
 * speed, and 9 gives best compression.
 * @return The size of the output in bytes,
 * @return 0 if INSIZE is 0, or
 * @return negative error code on error.
 *
 * @example
 * char in[8] = "abcdefg";
 * void *out;
 * int64_t out_size = MgzDeflate(&out, in, 8, 9);
 * if (out) {
 *     fwrite(out, 1, out_size, out_file);
 *     free(out);
 * }
 */
int64_t MgzDeflate(void **out, const void *in, int64_t in_size, int level);

/**
 * @brief Splits INSIZE bytes of data from IN into blocks of size BLOCKSIZE,
 * compresses each block using compression level LEVEL, and stores the
 * concatenated result to a malloc()ed array at mgz_res_t::out.
 *
 * @details The return structure contains all zeros if INSIZE
 * is 0 or an error occurs. Otherwise, it is the user's responsibility
 * to free() the output buffer at mgz_res_t::out.
 *
 * If LOOKUP is set to true, the returned mgz_res_t struct will also contain a
 * malloc()ed lookup table at mgz_res_t::lookup; otherwise mgz_res_t::lookup
 * will be set to NULL. It is the user's responsibility to free() the lookup
 * table if requested.
 *
 * The lookup table is an array of offsets of length (mgz_res_t::num_blocks +
 * 1). Each offset indicates the number of bytes that proceeds the given block
 * in the compressed buffer. For example, if there are 3 compressed blocks of
 * sizes 4, 6, and 2, then the lookup array is of length 4 with lookup[0] = 0,
 * lookup[1]  = 4, lookup[2] = 10, and lookup[3] = 12. The current version of
 * mgz does not store the last value into a standard lookup file.
 *
 * @param in Input.
 * @param in_size Size of the input in bytes.
 * @param level Gzip compression level which can be any integer from -1 to 9. -1
 * gives zlib's default compression level, 0 gives no compression, 1 gives best
 * speed, and 9 gives best compression.
 * @param block_size Size of each block of raw data in bytes. Minimum block size
 * is 16 KiB (1 << 14 bytes). The minimum block size is used if set to a value
 * smaller than the minimum block size. A default block size of 1 MiB is used if
 * set to 0.
 * @param lookup A lookup table is returned if set to true.
 * @return A mgz_res_t object containing the output buffer, the size of the
 * output buffer in bytes, the lookup table if requested, and the number of
 * blocks that raw data was split into.
 * @return If an error occurred during the compression, the returned object
 * contains all zeros. In particular, mgz_res_t::out is guaranteed to be
 * non-NULL on success or NULL if an error occurred.
 *
 * @example
 * mgz_res_t res =
 *     MgzParallelDeflate(in, in_size, level, block_size, true);
 * fwrite(res.out, 1, outSize, outfile);
 * free(res.out);
 * fwrite(&block_size, sizeof(int64_t), 1, lookupFile);
 * fwrite(res.lookup, sizeof(int64_t), res.num_blocks, lookupFile);
 * free(res.lookup);
 */
mgz_res_t MgzParallelDeflate(const void *in, int64_t in_size, int level,
                             int64_t block_size, bool lookup);

/**
 * @brief Splits INSIZE bytes of data from IN into blocks of size BLOCKSIZE,
 * compresses each block using compression level LEVEL, and writes the
 * concatenated result into OUTFILE assuming OUTFILE points to a valid writable
 * stream. Also writes the lookup table to LOOKUP if LOOKUP is not set to NULL,
 * in which case it is assumed to point to a valid writable stream.
 *
 * @param in Input.
 * @param size Size of the IN in bytes.
 * @param level Gzip compression level which can be any integer from -1 to 9. -1
 * gives zlib's default compression level, 0 gives no compression, 1 gives best
 * speed, and 9 gives best compression.
 * @param block_size Size of each block of raw data in bytes. Minimum block size
 * is 16 KiB (1 << 14 bytes). The minimum block size is used if set to a value
 * smaller than the minimum block size. A default block size of 1 MiB is used if
 * set to 0.
 * @param outfile Output stream to which the compressed data is written.
 * @param lookup Lookup stream to which the lookup table is written, or NULL if
 * no lookup table is needed.
 * @return Size written to OUTFILE in bytes,
 * @return 0 if SIZE is 0, or
 * @return negative error code if an error occurred during compression.
 */
int64_t MgzParallelCreate(const void *in, int64_t size, int level,
                          int64_t block_size, FILE *outfile, FILE *lookup);

/**
 * @brief Reads SIZE bytes of data into BUF from a gzip file created
 * with mgz, which has file descriptor FD, starting at offset OFFSET
 * using LOOKUP as lookup table. Assumes BUF points to a valid space
 * of size at least SIZE bytes, FD is a valid file descriptor of
 * a valid mgz gzip file, and LOOKUP points to a readable stream that
 * contains the lookup table for the given gzip file.
 *
 * @param buf output buffer.
 * @param size size of data to read from FD in bytes.
 * @param offset offset into FD in bytes.
 * @param fd file descriptor of a mgz gzip file.
 * @param lookup readable stream containing the lookup table for FD.
 * @return Number of bytes read from FD. Returns 0 if size is 0 or
 * an error occurred.
 */
int64_t MgzRead(void *buf, int64_t size, int64_t offset, int fd, FILE *lookup);

#endif  // GAMESMANEXPERIMENT_LIBS_MGZ_MGZ_H_

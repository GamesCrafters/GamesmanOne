/**
 * @file lz4_utils.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief LZ4 utilities
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

#ifndef GAMESMANONE_LIBS_LZ4_UTILS_LZ4_UTILS_H_
#define GAMESMANONE_LIBS_LZ4_UTILS_LZ4_UTILS_H_

#include <stddef.h>

// ============================ Types & Error Codes ============================

/**
 * @brief Error codes returned by the Lz4Utils API.
 */
typedef enum {
    LZ4_UTILS_SUCCESS = 0,          /**< Success */
    LZ4_UTILS_ERR_INVALID_PARAM,    /**< Invalid parameter(s) */
    LZ4_UTILS_ERR_OOM,              /**< Out of memory */
    LZ4_UTILS_ERR_IO,               /**< File I/O error */
    LZ4_UTILS_ERR_COMPRESS,         /**< Error during compression */
    LZ4_UTILS_ERR_CORRUPT_DATA,     /**< Corrupt data in compressed frame */
    LZ4_UTILS_ERR_INSUFFICIENT_BUF, /**< Insufficient buffer for output */
} Lz4UtilsStatus;

// ============================== Compression API ==============================

/**
 * @brief Concatenates and compresses `n` input streams using level `level`
 * LZ4 frame compression, and stores the compressed result as a file of name
 * `ofname`. If a file of name `ofname` already exists, it will be
 * overwritten.
 *
 * @param[in] in Array of `n` input buffers.
 * @param[in] in_sizes Array of `n` sizes, where `in_sizes[i]` is the size of
 * the i-th input buffer.
 * @param[in] n Number of input buffers in total.
 * @param[in] level LZ4 compression level.
 * @param[in] ofname Output file name.
 * @param[out] out_compressed_size Optional output parameter to receive the
 * size of the compressed file in bytes on success. Unmodified on error. Pass
 * `NULL` to omit this output.
 *
 * @retval LZ4_UTILS_SUCCESS on success.
 * @retval LZ4_UTILS_ERR_INVALID_PARAM if any of the parameters are invalid.
 * Cases include:
 *     1. Either `in` or `in_sizes` is `NULL` while `n` is non-zero;
 *     2. `in[i]` is `NULL` but `in_sizes[i]` is non-zero for any i in range
 *        [0, n);
 *     3. `ofname` is `NULL`.
 * @retval LZ4_UTILS_ERR_OOM if failed to allocate memory for compression.
 * @retval LZ4_UTILS_ERR_IO if failed to create or write to the output file.
 * @retval LZ4_UTILS_ERR_COMPRESS if an error occurred during compression.
 */
Lz4UtilsStatus Lz4UtilsCompressBuffersToFile(const void *const *in,
                                             const size_t *in_sizes, int n,
                                             int level, const char *ofname,
                                             size_t *out_compressed_size);

/**
 * @brief Compresses `in_size` bytes of `in` using level `level` LZ4 frame
 * compression, and stores the compressed stream as a file of name `ofname`.
 * If a file of name `ofname` already exists, it will be overwritten.
 *
 * @param[in] in Input buffer.
 * @param[in] in_size Size of the input buffer.
 * @param[in] level LZ4 compression level.
 * @param[in] ofname Output file name.
 * @param[out] out_compressed_size Optional output parameter to receive the
 * size of the compressed file in bytes on success. Unmodified on error. Pass
 * `NULL` to omit this output.
 *
 * @retval LZ4_UTILS_SUCCESS on success.
 * @retval LZ4_UTILS_ERR_INVALID_PARAM if `in` is `NULL` but `in_size` is
 * non-zero, or if `ofname` is `NULL`.
 * @retval LZ4_UTILS_ERR_OOM if failed to allocate memory for compression.
 * @retval LZ4_UTILS_ERR_IO if failed to create or write to the output file.
 * @retval LZ4_UTILS_ERR_COMPRESS if an error occurred during compression.
 */
Lz4UtilsStatus Lz4UtilsCompressBufferToFile(const void *in, size_t in_size,
                                            int level, const char *ofname,
                                            size_t *out_compressed_size);

/**
 * @brief Compresses the input file of name `ifname` using level `level` LZ4
 * frame compression, and stores the compressed stream as a file of name
 * `ofname`.
 *
 * @param[in] ifname Input file name.
 * @param[in] level LZ4 compression level.
 * @param[in] ofname Output file name.
 * @param[out] out_compressed_size Optional output parameter to receive the
 * size of the compressed file in bytes on success. Unmodified on error. Pass
 * `NULL` to omit this output.
 *
 * @retval LZ4_UTILS_SUCCESS on success.
 * @retval LZ4_UTILS_ERR_INVALID_PARAM if `ifname` or `ofname` is `NULL`.
 * @retval LZ4_UTILS_ERR_IO if failed to read from the input file, or failed to
 * create or write to the output file.
 * @retval LZ4_UTILS_ERR_OOM if failed to allocate memory for compression.
 * @retval LZ4_UTILS_ERR_COMPRESS if an error occurred during compression.
 */
Lz4UtilsStatus Lz4UtilsCompressFileToFile(const char *ifname, int level,
                                          const char *ofname,
                                          size_t *out_compressed_size);

// ========================= Streaming Compression API =========================

/**
 * @brief Opaque object for data passing when streaming to a Lz4Utils
 * compressed file for chunked compression.
 */
typedef struct Lz4UtilsOutStream Lz4UtilsOutStream;

/**
 * @brief Creates a new Lz4Utils output stream.
 *
 * @param[in] ofname Output file name.
 * @param[in] level LZ4 compression level.
 *
 * @returns Pointer to the new Lz4Utils output stream on success. `NULL` on
 * failure (e.g., file creation or memory allocation failed).
 */
Lz4UtilsOutStream *Lz4UtilsOutStreamCreate(const char *ofname, int level);

/**
 * @brief Runs compression to consume `in_size` bytes of data from `in` using
 * `stream` as output stream.
 *
 * @param[in,out] stream Output stream.
 * @param[in] in Pointer to the input buffer.
 * @param[in] in_size Number of bytes to consume from the input buffer.
 * @param[out] out_bytes_written Optional output parameter to receive the
 * number of compressed bytes generated and written in this run on success.
 * Unmodified on error. Pass `NULL` to omit this output.
 *
 * @retval LZ4_UTILS_SUCCESS on success.
 * @retval LZ4_UTILS_ERR_INVALID_PARAM if `stream` is `NULL`, or if `in` is
 * `NULL` but `in_size` is greater than zero.
 * @retval LZ4_UTILS_ERR_COMPRESS if internal LZ4 compression failed.
 * @retval LZ4_UTILS_ERR_IO if failed to write compressed bytes to the output
 * file.
 */
Lz4UtilsStatus Lz4UtilsOutStreamRun(Lz4UtilsOutStream *stream, const void *in,
                                    size_t in_size, size_t *out_bytes_written);

/**
 * @brief Closes the output stream `stream`, finalizing the output file by
 * flushing all buffered compressed bytes to disk. Does nothing and returns
 * `LZ4_UTILS_SUCCESS` if `stream` is `NULL`.
 *
 * @param[in,out] stream Output stream to close.
 * @param[out] out_total_compressed_size Optional output parameter to receive
 * the number of compressed bytes generated in total since the creation of
 * `stream` on success. Unmodified on error. Pass `NULL` to omit this output.
 *
 * @retval LZ4_UTILS_SUCCESS on success.
 * @retval LZ4_UTILS_ERR_COMPRESS if internal LZ4 compression/finalization
 * failed.
 * @retval LZ4_UTILS_ERR_IO if failed to write finalized compressed bytes to the
 * output file.
 */
Lz4UtilsStatus Lz4UtilsOutStreamClose(Lz4UtilsOutStream *stream,
                                      size_t *out_total_compressed_size);

// ============================= Decompression API =============================

/**
 * @brief Decompresses the input file of name `ifname`, which is assumed to
 * contain exactly one LZ4 frame compressed from `n` input buffers of sizes
 * specified by `out_sizes`, and stores the uncompressed streams in `out`.
 *
 * @param[in] ifname Input compressed file name.
 * @param[out] out Array of output buffers.
 * @param[in] out_sizes Array of output buffer sizes, where `out_sizes[i]` is
 * the size of the i-th buffer.
 * @param[in] n Number of output buffers.
 * @param[out] out_uncompressed_size Optional output parameter to receive the
 * number of bytes of uncompressed data written to all output buffers in total
 * on success. Unmodified on error. Pass `NULL` to omit this output.
 *
 * @retval LZ4_UTILS_SUCCESS on success.
 * @retval LZ4_UTILS_ERR_INVALID_PARAM if
 *     1. `n` is smaller than 0;
 *     2. `n` is greater than 0 but `ifname`, `out`, or `out_sizes` is `NULL`;
 *     3. `out[i]` is `NULL` when `out_sizes[i] > 0` for any i in range [0, n).
 * @retval LZ4_UTILS_ERR_IO if failed to open the input file, or an error
 * occurred when reading it.
 * @retval LZ4_UTILS_ERR_OOM if failed to allocate memory.
 * @retval LZ4_UTILS_ERR_CORRUPT_DATA if the input file is corrupt.
 * @retval LZ4_UTILS_ERR_INSUFFICIENT_BUF if failed to decompress due to not
 * enough output buffer capacity, or if the decompressed data is larger than
 * the size of all output buffers combined.
 */
Lz4UtilsStatus Lz4UtilsDecompressFileToBuffers(const char *ifname,
                                               void *const *out,
                                               const size_t *out_sizes, int n,
                                               size_t *out_uncompressed_size);

/**
 * @brief Decompresses the input file of name `ifname`, which is assumed to
 * contain exactly one LZ4 frame, and stores the uncompressed data in `out` of
 * capacity `out_size`.
 *
 * @param[in] ifname Input compressed file name.
 * @param[out] out Output buffer.
 * @param[in] out_size Size capacity of the output buffer.
 * @param[out] out_uncompressed_size Optional output parameter to receive the
 * number of bytes of uncompressed data written to `out` on success. Unmodified
 * on error. Pass `NULL` to omit this output.
 *
 * @retval LZ4_UTILS_SUCCESS on success.
 * @retval LZ4_UTILS_ERR_INVALID_PARAM if `ifname` is `NULL`, or if `out` is
 * `NULL` when `out_size` is greater than 0.
 * @retval LZ4_UTILS_ERR_IO if failed to open the input file, or an error
 * occurred when reading it.
 * @retval LZ4_UTILS_ERR_OOM if failed to allocate memory.
 * @retval LZ4_UTILS_ERR_CORRUPT_DATA if the input file is corrupt.
 * @retval LZ4_UTILS_ERR_INSUFFICIENT_BUF if failed to decompress due to not
 * enough output buffer capacity, or if the decompressed data is larger than
 * the size of the output buffer.
 */
Lz4UtilsStatus Lz4UtilsDecompressFileToBuffer(const char *ifname, void *out,
                                              size_t out_size,
                                              size_t *out_uncompressed_size);

// ======================== Streaming Decompression API ========================

/**
 * @brief Opaque object for data passing when streaming from a Lz4Utils
 * compressed file for chunked decompression.
 */
typedef struct Lz4UtilsInStream Lz4UtilsInStream;

/**
 * @brief Creates a new Lz4Utils input stream.
 *
 * @param[in] ifname Input file name.
 *
 * @returns Pointer to the new Lz4Utils input stream on success. `NULL` on
 * failure (e.g., file open or memory allocation failed).
 */
Lz4UtilsInStream *Lz4UtilsInStreamCreate(const char *ifname);

/**
 * @brief Decompresses up to `out_size` bytes of data to `out` using `stream`
 * as input stream.
 *
 * @param[in,out] stream Input stream.
 * @param[out] out Output buffer to receive decompressed data.
 * @param[in] out_size Capacity of the output buffer.
 * @param[out] out_uncompressed_size Optional output parameter to receive the
 * number of uncompressed bytes generated in this run on success. It is set to 0
 * on EOF, or unmodified on error. Pass `NULL` to omit this output.
 *
 * @retval LZ4_UTILS_SUCCESS on success (including EOF).
 * @retval LZ4_UTILS_ERR_INVALID_PARAM if `stream` or `out` is `NULL`.
 * @retval LZ4_UTILS_ERR_CORRUPT_DATA if the compressed file associated with
 * `stream` is malformed, or on internal LZ4 decompression failure.
 * @retval LZ4_UTILS_ERR_IO if a file reading error occurs mid-stream.
 */
Lz4UtilsStatus Lz4UtilsInStreamRun(Lz4UtilsInStream *stream, void *out,
                                   size_t out_size,
                                   size_t *out_uncompressed_size);

/**
 * @brief Closes the input stream `stream` and deallocates associated
 * resources.
 *
 * @param[in,out] stream Input stream to close. Supports `NULL`.
 */
void Lz4UtilsInStreamClose(Lz4UtilsInStream *stream);

#endif  // GAMESMANONE_LIBS_LZ4_UTILS_LZ4_UTILS_H_

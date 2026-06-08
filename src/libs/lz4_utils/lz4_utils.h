/**
 * @file lz4_utils.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief LZ4 utilities
 * @version 0.2.1
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

#ifndef GAMESMANONE_LIBS_LZ4_UTILS_LZ4_UTILS_H_
#define GAMESMANONE_LIBS_LZ4_UTILS_LZ4_UTILS_H_

#include <stddef.h>  // size_t
#include <stdint.h>  // int64_t

// ============================== Compression API ==============================

/**
 * @brief Concatenates and compresses \p n input streams using level \p level
 * LZ4 frame compression, stores the compressed result as file of name \p
 * ofname, and returns the size of the compressed file in bytes on success. If a
 * file of name \p ofname already exists, it will be overwritten.
 *
 * @param in Array of \p n input buffers.
 * @param in_sizes Array of \p n sizes, where in_sizes[i] is the size of the
 * i-th input buffer.
 * @param n Number of input buffers in total.
 * @param level LZ4 compression level.
 * @param ofname Output file name.
 * @return Size of the compressed file on success,
 * @return -1 if either \p in or \p in_sizes is \c NULL but \p n is non-zero OR
 * if in[i] is NULL but in_sizes[i] is non-NULL for any i in range [0, n);
 * @return -2 if failed to allocate memory for compression; or
 * @return -3 if failed to create or write to the output file.
 */
int64_t Lz4UtilsCompressStreams(const void *const *in, const size_t *in_sizes,
                                int n, int level, const char *ofname);

/**
 * @brief Compresses \p in_size bytes of \p in using level \p level LZ4 frame
 * compression, stores the compressed stream as file of name \p ofname, and
 * returns the size of the compressed file in bytes on success. If a file of
 * name \p ofname already exists, it will be overwritten.
 *
 * @param in Input buffer.
 * @param in_size Size of the input buffer.
 * @param level LZ4 compression level.
 * @param ofname Output file name.
 * @return Size of the compressed file on success,
 * @return -1 if \p in is \c NULL but \p in_size is non-zero,
 * @return -2 if failed to allocate memory for compression, or
 * @return -3 if failed to create or write to the output file.
 */
int64_t Lz4UtilsCompressStream(const void *in, size_t in_size, int level,
                               const char *ofname);

/**
 * @brief Compresses the input file of name \p ifname using level \p level LZ4
 * frame compression, stores the compressed stream as file of name \p ofname,
 * and returns the size of the compressed file in bytes on success.
 *
 * @param ifname Input file name.
 * @param level LZ4 compression level.
 * @param ofname Output file name.
 * @return Size of the compressed file on success,
 * @return -1 if failed to read from the input file,
 * @return -2 if failed to allocate memory for compression, or
 * @return -3 if failed to create or write to the output file.
 */
int64_t Lz4UtilsCompressFile(const char *ifname, int level, const char *ofname);

// ========================= Streaming Compression API =========================

/**
 * @brief Opaque object for data passing when streaming to a Lz4Utils
 * compresssed file.
 */
typedef struct Lz4UtilsOutStream Lz4UtilsOutStream;

/**
 * @brief Creates a new Lz4Utils output stream.
 *
 * @param ofname Output file name.
 * @param level LZ4 compression level.
 * @return Pointer to the new Lz4Utils output stream on success, or
 * @return \c NULL on failure.
 */
Lz4UtilsOutStream *Lz4UtilsOutStreamCreate(const char *ofname, int level);

/**
 * @brief Runs compression to consume \p in_size bytes of data from \p in using
 * \p stream as output stream.
 *
 * @param stream Output stream.
 * @param stream Output stream.
 * @param in Pointer to the input buffer.
 * @param in_size Number of bytes to consume from the input buffer.
 * @return Number of compressed bytes generated in this run on success,
 * @return -2 if internal LZ4 compression failed, or
 * @return -3 if failed to write compressed bytes to output file.
 */
int64_t Lz4UtilsOutStreamRun(Lz4UtilsOutStream *stream, const void *in,
                             size_t in_size);

/**
 * @brief Closes the output stream \p stream , finalizing the output file by
 * flushing all buffered compressed bytes to disk. Does nothing and returns
 * 0 if \p stream is \c NULL .
 *
 * @param stream Output stream to close.
 * @return Number of compressed bytes generated in total since the creation of
 * \p stream on success,
 * @return -2 if internal LZ4 compression failed, or
 * @return -3 if failed to write compressed bytes to output file.
 */
int64_t Lz4UtilsOutStreamClose(Lz4UtilsOutStream *stream);

// ============================= Decompression API =============================

/**
 * @brief Decompresses the input file of name \p ifname, which is assumed to
 * contain exactly one LZ4 frame compressed from \p n input buffers of sizes
 * specified by \p out_sizes, and stores the uncompresses streams in \p out.
 *
 * @param ifname Input compressed file name.
 * @param out (Output parameter) array of output buffers.
 * @param out_sizes Array of output buffer sizes, where out_sizes[i] is the size
 * of the i-th buffer.
 * @param n Number of output buffers.
 * @return Number of bytes of uncompressed data written to all output buffers in
 * total;
 * @return -1 if failed to open input file;
 * @return -2 if failed to allocate memory;
 * @return -3 if input file is corrupt or an error occurred when reading the
 * input file;
 * @return -4 if failed to decompress due to, for example, not enough output
 * buffer capacity;
 * @return -5 if the decompressed data is larger than the size of all output
 * buffers combined.
 */
int64_t Lz4UtilsDecompressFileMultistream(const char *ifname, void **out,
                                          const size_t *out_sizes, int n);

/**
 * @brief Decompresses the input file of name \p ifname, which is assumed to
 * contain exactly one LZ4 frame, and stores the uncompresses data in \p out of
 * size \p out_size.
 *
 * @param ifname Input compressed file name.
 * @param out (Output parameter) output buffer.
 * @param out_size Size of the output buffer.
 * @return Number of bytes of uncompressed data written to \p out;
 * @return -1 if failed to open input file;
 * @return -2 if failed to allocate memory;
 * @return -3 if input file is corrupt or an error occurred when reading the
 * input file;
 * @return -4 if failed to decompress due to, for example, not enough output
 * buffer capacity;
 * @return -5 if the decompressed data is larger than the size of the output
 * buffer.
 */
int64_t Lz4UtilsDecompressFile(const char *ifname, void *out, size_t out_size);

// ======================== Streaming Decompression API ========================

/**
 * @brief Opaque object for data passing when streaming from a Lz4Utils
 * compresssed file.
 */
typedef struct Lz4UtilsInStream Lz4UtilsInStream;

/**
 * @brief Creates a new Lz4Utils input stream.
 *
 * @param ifname Input file name.
 * @return Pointer to the new Lz4Utils input stream on success, or
 * @return \c NULL on failure.
 */
Lz4UtilsInStream *Lz4UtilsInStreamCreate(const char *ifname);

/**
 * @brief Decompress X bytes of data to \p out using \p stream as input stream,
 * where X is \p out_size or the amount of compressed data left in \p stream ,
 * whichever is smaller.
 *
 * @param stream
 * @param out
 * @param out_size
 * @return Number of uncompressed bytes generated in this run on success,
 * @return 0 if no bytes are left in \p stream ,
 * @return -1 if \p stream is \c NULL ,
 * @return -3 if the compressed file associated with \p stream is malformed, or
 * @return -4 on internal LZ4 decompression failure.
 */
int64_t Lz4UtilsInStreamRun(Lz4UtilsInStream *stream, void *out,
                            size_t out_size);

/**
 * @brief Closes the input stream \p stream .
 *
 * @param stream Input stream to close.
 * @return 0 on success,
 * @return -1 if on internal LZ4 context deallocation failure.
 */
int Lz4UtilsInStreamClose(Lz4UtilsInStream *stream);

#endif  // GAMESMANONE_LIBS_LZ4_UTILS_LZ4_UTILS_H_

/**
 * @file xzra.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief XZ utilities with random access.
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

#ifndef GAMESMANONE_LIBS_XZRA_XZRA_H_
#define GAMESMANONE_LIBS_XZRA_XZRA_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// ============================== Common API ==============================

/**
 * @brief Return status codes for the XZRA library.
 */
typedef enum {
    XZRA_SUCCESS = 0,
    XZRA_ERR_IN_FILE,
    XZRA_ERR_OUT_FILE,
    XZRA_ERR_CODEC,
    XZRA_ERR_CLOSE,
    XZRA_ERR_INVALID_PARAM,
} XzraStatus;

/**
 * @brief Compression and decompression options.
 */
typedef struct {
    /**
     * Size of each uncompressed block.
     */
    uint64_t block_size;

    /**
     * Compression level from 0 (store) to 9 (ultra).
     */
    uint32_t level;

    /**
     * Extreme compression mode will be enabled if this parameter is set to
     * true.
     */
    bool extreme;

    /**
     * Number of threads to use. 0 detects and uses physical threads available
     * while falling back to 1 if detection fails.
     */
    int num_threads;
} XzraCodecOptions;

// ============================== Compression API ==============================

/**
 * @brief Calculates the memory usage required by the XZRA compressor.
 *
 * @param[in] options Pointer to the compression options.
 * @param[out] out_mem_usage Pointer to store the memory requirement in bytes.
 *
 * @retval XZRA_SUCCESS On success.
 * @retval XZRA_ERR_INVALID_PARAM If the given options are invalid.
 */
XzraStatus XzraCompressionMemUsage(const XzraCodecOptions *options,
                                   uint64_t *out_mem_usage);

/**
 * @brief Compresses an input file using a single LZMA2 filter.
 *
 * @details The input file `ifname` is divided into blocks of `block_size` bytes
 * and compressed in parallel using `num_threads` threads. Because blocks are
 * independent, the resulting file allows random access if `block_size` is small
 * enough. Compression uses a dictionary equal to the block size. While smaller
 * block sizes enable faster random access, they can worsen the compression
 * ratio. XZ Utils enforces a minimum block size of 4 KiB and recommends at
 * least 1 MiB for reasonable compression.
 *
 * @param[in] ofname Output XZ file name.
 * @param[in] ifname Input file name.
 * @param[in] options Pointer to the compression options.
 * @param[out] out_size Pointer to store the output file size in bytes.
 *
 * @retval XZRA_SUCCESS On success.
 * @retval XZRA_ERR_IN_FILE If the input file cannot be opened.
 * @retval XZRA_ERR_OUT_FILE If the output file cannot be accessed or written.
 * @retval XZRA_ERR_CODEC If compression fails.
 */
XzraStatus XzraCompressFile(const char *ofname, const char *ifname,
                            const XzraCodecOptions *options,
                            uint64_t *out_size);

/**
 * @brief Compresses a single memory chunk using a single LZMA2 filter.
 *
 * @details The memory buffer `in` is divided into blocks of `block_size` bytes
 * and compressed in parallel using `num_threads` threads. Independent blocks
 * allow for random access within the compressed stream if `block_size` is small
 * enough. Smaller block sizes favor fast random access at the cost of a lower
 * compression ratio. XZ Utils requires a minimum block size of 4 KiB and
 * recommends at least 1 MiB for efficient compression.
 *
 * @param[in] ofname Output XZ file name.
 * @param[in] in Pointer to the input memory buffer.
 * @param[in] in_size Size of the input buffer in bytes.
 * @param[in] options Pointer to the compression options.
 * @param[out] out_size Pointer to store the output file size in bytes.
 *
 * @retval XZRA_SUCCESS On success.
 * @retval XZRA_ERR_OUT_FILE If the output file cannot be created or opened.
 * @retval XZRA_ERR_CODEC If compression fails.
 */
XzraStatus XzraCompressMem(const char *ofname, const uint8_t *in,
                           size_t in_size, const XzraCodecOptions *options,
                           uint64_t *out_size);

// =============================== Streaming API ===============================

/**
 * @brief Opaque object for data passing when streaming to an XZRA file.
 */
typedef struct XzraOutStream XzraOutStream;

/**
 * @brief Creates a new XZRA output stream.
 *
 * @param[in] ofname Output file name.
 * @param[in] options Pointer to the compression options.
 *
 * @returns A pointer to the new output stream.
 * @retval NULL On failure.
 */
XzraOutStream *XzraOutStreamCreate(const char *ofname,
                                   const XzraCodecOptions *options);

/**
 * @brief Consumes data from an input buffer and compresses it into the stream.
 *
 * @param[in,out] stream Pointer to the output stream.
 * @param[in] in Pointer to the input buffer.
 * @param[in] in_size Number of bytes to consume from the input buffer.
 * @param[out] out_bytes_written Pointer to store the generated bytes count.
 *
 * @retval XZRA_SUCCESS On success.
 * @retval XZRA_ERR_OUT_FILE If the output file cannot be written to.
 * @retval XZRA_ERR_CODEC On compression failure.
 */
XzraStatus XzraOutStreamRun(XzraOutStream *stream, const uint8_t *in,
                            size_t in_size, uint64_t *out_bytes_written);

/**
 * @brief Finalizes the output file by flushing buffered bytes and closes it.
 *
 * @details Does nothing and returns `XZRA_SUCCESS` if `stream` is `NULL`.
 *
 * @param[in,out] stream Output stream to close.
 * @param[out] out_total_bytes Pointer to store the total generated bytes.
 *
 * @retval XZRA_SUCCESS On success.
 * @retval XZRA_ERR_OUT_FILE If the file cannot be written to or closed.
 * @retval XZRA_ERR_CODEC On compression failure.
 */
XzraStatus XzraOutStreamClose(XzraOutStream *stream, uint64_t *out_total_bytes);

/**
 * @brief Safely frees stream memory without finalizing or flushing to disk.
 *
 * @details This function is useful for cleaning up resources when an error
 * occurs midway through a streaming process.
 *
 * @param[in,out] stream Output stream to abort.
 */
void XzraOutStreamAbort(XzraOutStream *stream);

// ============================= Decompression API =============================

/**
 * @brief Calculates the memory usage required by the XZRA decompressor.
 *
 * @param[in] options Compression options used during encoding.
 * @param[out] out_mem_usage Pointer to store the memory requirement in bytes.
 *
 * @retval XZRA_SUCCESS On success.
 * @retval XZRA_ERR_INVALID_PARAM If the given options are invalid.
 */
XzraStatus XzraDecompressionMemUsage(const XzraCodecOptions *options,
                                     uint64_t *out_mem_usage);

/**
 * @brief Decompresses a specified number of bytes from an XZ file.
 *
 * @details The function writes at most `size` bytes into `dest`. If the true
 * uncompressed size is smaller than `size`, only the available bytes are
 * decoded, and `out_decompressed_size` reflects this actual count. The function
 * dynamically scales threads based on the file's properties and the `memlimit`.
 * Note that `memlimit` excludes `size`, so the caller must independently factor
 * in the size of the destination buffer when budgeting memory.
 *
 * @param[out] dest Buffer to hold the decompressed output.
 * @param[in] filename Name of the input XZ file.
 * @param[in] size Maximum number of bytes to decompress.
 * @param[in] num_threads Thread count (0 to auto-detect based on hardware).
 * @param[in] memlimit Decoder memory limit in bytes.
 * @param[out] out_decompressed_size Pointer to store the actual bytes decoded.
 *
 * @retval XZRA_SUCCESS On success.
 * @retval XZRA_ERR_IN_FILE If the file fails to open.
 * @retval XZRA_ERR_CODEC If an error occurs during decompression.
 * @retval XZRA_ERR_CLOSE If the file fails to close properly.
 * @retval XZRA_ERR_INVALID_PARAM If threads, memory, or limits are invalid.
 */
XzraStatus XzraDecompressFile(uint8_t *dest, const char *filename, size_t size,
                              int num_threads, uint64_t memlimit,
                              uint64_t *out_decompressed_size);

/**
 * @brief Read-only XZ file with random access.
 */
typedef struct XzraFile XzraFile;

/**
 * @brief Options for the origin parameter of `XzraFileSeek`.
 */
enum XzraSeekOrigin {
    XZRA_SEEK_SET = 0, /**< Seek from the beginning of the file. */
    XZRA_SEEK_CUR = 1, /**< Seek from the current position. */
};

/**
 * @brief Opens a read-only `XzraFile`.
 *
 * @param[in] filename Name of the target XZ file.
 *
 * @returns A pointer to the opened file.
 * @retval NULL If the given `filename` cannot be opened.
 */
XzraFile *XzraFileOpen(const char *filename);

/**
 * @brief Closes the given `XzraFile`.
 *
 * @details Does nothing if `file` is `NULL`.
 *
 * @param[in,out] file File to close.
 *
 * @retval 0 On success.
 * @retval EOF On failure.
 */
int XzraFileClose(XzraFile *file);

/**
 * @brief Sets the internal position indicator for the uncompressed file.
 *
 * @details The EOF flag of `file` is cleared on a successful call regardless
 * of the bounds. The system does not eagerly verify if the new `offset` exceeds
 * the file length until a read occurs.
 *
 * @param[in,out] file Target file.
 * @param[in] offset Offset relative to `origin` in uncompressed bytes.
 * @param[in] origin `XZRA_SEEK_SET` (absolute) or `XZRA_SEEK_CUR` (relative).
 *
 * @retval 0 On success.
 * @retval -1 If `origin` is invalid.
 */
int XzraFileSeek(XzraFile *file, int64_t offset, int origin);

/**
 * @brief Reads up to `size` uncompressed bytes into `dest`.
 *
 * @details If EOF is reached before fulfilling `size`, the function reads what
 * it can, sets the EOF flag to true, and returns the successfully read count.
 * Callers should check if the returned count equals `size` to verify a complete
 * read, or rely on internal length tracking.
 *
 * @param[out] dest Destination buffer for the uncompressed data.
 * @param[in] size Number of bytes to read.
 * @param[in,out] file Source XZ file.
 *
 * @returns The number of bytes successfully read.
 */
size_t XzraFileRead(void *dest, size_t size, XzraFile *file);

/**
 * @brief Returns whether the end of the uncompressed file has been reached.
 *
 * @details When the EOF flag is true, subsequent calls to `XzraFileRead`
 * return 0 until the flag is cleared by `XzraFileSeek`.
 *
 * @param[in] file Target file.
 *
 * @retval true If EOF has been reached.
 * @retval false Otherwise.
 */
bool XzraFileEOF(const XzraFile *file);

#endif  // GAMESMANONE_LIBS_XZRA_XZRA_H_

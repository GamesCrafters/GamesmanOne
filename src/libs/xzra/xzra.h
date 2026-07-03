/**
 * @file xzra.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief XZ utilities with random access.
 * @version 2.0.0
 * @date 2025-06-08
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
    /** Size of each uncompressed block. */
    uint64_t block_size;
    /** Compression level from 0 (store) to 9 (ultra). */
    uint32_t level;
    /** Extreme compression mode will be enabled if this parameter is set to
     * true. */
    bool extreme;
    /** Number of threads to use. 0 detects and uses physical threads available
     * while falling back to 1 if detection fails.
     */
    int num_threads;
} XzraCodecOptions;

// ============================== Compression API ==============================

/**
 * @brief Returns the memory usage (in bytes) of the XZRA compressor using the
 * given compression options.
 *
 * @param options Pointer to the compression options.
 * @param out_mem_usage Pointer to store the number of bytes of memory required.
 * @return XZRA_SUCCESS on success;
 * @return XZRA_ERR_INVALID_PARAM if the given options are invalid.
 */
XzraStatus XzraCompressionMemUsage(const XzraCodecOptions *options,
                                   uint64_t *out_mem_usage);

/**
 * @brief Compresses input file of name \p ifname using a single LZMA2 filter
 * and stores the output XZ stream in output file of name \p ofname.
 *
 * @details The input file is first divided into blocks each of \p block_size
 * bytes, and then compressed in parallel using \p num_threads threads. Blocks
 * are independent of each other, thus allowing random access to the compressed
 * stream if \p block_size is sufficiently small. The compression of each block
 * uses a dictionary of size equal to the size of the block to minimize
 * compressed size. Since the purpose of this library is to provide fast random
 * access to XZ files, using a large dictionary should be okay as we are
 * assuming \p block_size is small. Note that the setting of \p block_size also
 * affects compression ratio. In general, compression ratio deteriorates as
 * \p block_size decreases. XZ Utils enforces a minimum block size of 4 KiB and
 * recommends a minimum block size of 1 MiB for a reasonably good compression
 * ratio.
 *
 * @param ofname Output file name.
 * @param ifname Input file name.
 * @param options Pointer to the compression options.
 * @param out_size Pointer to store the size of the output file in bytes.
 * @return XZRA_SUCCESS on success;
 * @return XZRA_ERR_IN_FILE if the input file cannot be opened;
 * @return XZRA_ERR_OUT_FILE if the output file cannot be created, opened,
 * written to, or properly closed;
 * @return XZRA_ERR_CODEC if compression failed.
 */
XzraStatus XzraCompressFile(const char *ofname, const char *ifname,
                            const XzraCodecOptions *options,
                            uint64_t *out_size);

/**
 * @brief Compresses a single consecutive chunk of memory of size \p in_size
 * bytes pointed by \p in using a single LZMA2 filter and stores the output XZ
 * stream in a single output file of name \p ofname.
 *
 * @details The input is first divided into blocks each of \p block_size bytes,
 * and then compressed in parallel using \p num_threads threads. Blocks are
 * independent of each other, thus allowing random access to the compressed
 * stream if \p block_size is sufficiently small. The compression of each block
 * uses a dictionary of size equal to the size of the block to minimize
 * compressed size. Since the purpose of this library is to provide fast random
 * access to XZ files, using a large dictionary should be okay as we are
 * assuming \p block_size to be small. Note that the setting of \p block_size
 * also affects compression ratio. In general, compression ratio deteriorates as
 * \p block_size decreases. XZ Utils enforces a minimum block size of 4 KiB and
 * recommends a minimum block size of 1 MiB for a reasonably good compression
 * ratio.
 *
 * @param ofname Output file name.
 * @param in Input stream.
 * @param in_size Size of the input stream in bytes.
 * @param options Pointer to the compression options.
 * @param out_size Pointer to store the size of the output file in bytes.
 * @return XZRA_SUCCESS on success;
 * @return XZRA_ERR_OUT_FILE if the output file cannot be created or opened;
 * @return XZRA_ERR_CODEC if compression failed.
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
 * @param ofname Output file name.
 * @param options Pointer to the compression options.
 * @return Pointer to the new XZRA output stream on success, or
 * @return \c NULL on failure.
 */
XzraOutStream *XzraOutStreamCreate(const char *ofname,
                                   const XzraCodecOptions *options);

/**
 * @brief Runs compression to consume \p in_size bytes of data from \p in using
 * \p stream as output stream.
 *
 * @param stream Output stream.
 * @param in Pointer to the input buffer.
 * @param in_size Number of bytes to consume from the input buffer.
 * @param out_bytes_written Pointer to store the number of compressed bytes
 * generated in this run.
 * @return XZRA_SUCCESS on success;
 * @return XZRA_ERR_OUT_FILE if the output file cannot be written to;
 * @return XZRA_ERR_CODEC on failure.
 */
XzraStatus XzraOutStreamRun(XzraOutStream *stream, const uint8_t *in,
                            size_t in_size, uint64_t *out_bytes_written);

/**
 * @brief Closes the output stream \p stream , finalizing the output file by
 * flushing all buffered compressed bytes to disk. Does nothing and returns
 * XZRA_SUCCESS if \p stream is \c NULL .
 *
 * @param stream Output stream to close.
 * @param out_total_bytes Pointer to store the total number of compressed bytes
 * generated.
 * @return XZRA_SUCCESS on success;
 * @return XZRA_ERR_OUT_FILE if the output file cannot be written to or properly
 * closed;
 * @return XZRA_ERR_CODEC on failure.
 */
XzraStatus XzraOutStreamClose(XzraOutStream *stream, uint64_t *out_total_bytes);

/**
 * @brief Safely frees the stream memory without finalizing or flushing data to
 * disk. Useful for cleaning up resources when an error occurs halfway through
 * streaming.
 *
 * @param stream Output stream to abort.
 */
// void XzraOutStreamAbort(XzraOutStream *stream);

// ============================= Decompression API =============================

/**
 * @brief Returns the memory usage (in bytes) of the XZRA decompressor.
 *
 * @param options Pointer to the compression options used during encoding.
 * @param out_mem_usage Pointer to store the number of bytes of memory required.
 * @return XZRA_SUCCESS on success;
 * @return XZRA_ERR_INVALID_PARAM if the given options are invalid.
 */
XzraStatus XzraDecompressionMemUsage(const XzraCodecOptions *options,
                                     uint64_t *out_mem_usage);

/**
 * @brief Decompresses at most \p size bytes of the input XZ file of name
 * \p filename into \p dest using \p num_threads threads and at most \p memlimit
 * bytes of memory.
 *
 * @details If the uncompressed size of the specified file is smaller than
 * \p size bytes, only X bytes will be decompressed and the function will
 * return X, where X is the uncompressed size of the file in bytes.
 *
 * The function may decide to use fewer than \p num_threads threads depending on
 * the properties of the input file (e.g., dictionary size) and \p memlimit to
 * not use more memory than required. If, however, the decompression cannot be
 * completed using no more than \p memlimit bytes of memory even on a single
 * thread, the decompression will fail and XZRA_ERR_INVALID_PARAM will be
 * returned. Note that \p memlimit does not include \p size, and it is the
 * caller's responsibility to take the output buffer size into account when
 * calculating memory usage.
 *
 * @param dest Destination buffer, which is assumed to be of size at least
 * \p size bytes.
 * @param filename Name of the input file to decompress.
 * @param size Number of bytes to decompress.
 * @param num_threads Number of threads to use. 0 detects and uses physical
 * threads available.
 * @param memlimit Memory limit of the function in bytes.
 * @param out_decompressed_size Pointer to store the number of bytes
 * successfully decompressed.
 * @return XZRA_SUCCESS on success;
 * @return XZRA_ERR_IN_FILE if failed to open file;
 * @return XZRA_ERR_CODEC if an error occurred during decompression;
 * @return XZRA_ERR_CLOSE if failed to close the file;
 * @return XZRA_ERR_INVALID_PARAM if failed due to invalid \p num_threads,
 * \p memlimit, or out of memory.
 */
XzraStatus XzraDecompressFile(uint8_t *dest, const char *filename, size_t size,
                              int num_threads, uint64_t memlimit,
                              uint64_t *out_decompressed_size);

/** @brief Read-only XZ file with random access. */
typedef struct XzraFile XzraFile;

/** @brief Options for the third parameter of XzraFileSeek. */
enum XzraSeekOrigin {
    XZRA_SEEK_SET = 0, /* Seek from beginning of file.  */
    XZRA_SEEK_CUR = 1, /* Seek from current position.  */
};

/**
 * @brief Opens a read-only \c XzraFile of name \p filename.
 *
 * @param filename Name of the XZ file.
 * @return Pointer to the opened file, which must be closed using the provided
 * \c XzraFileClose function;
 * @return \c NULL if the given \p filename cannot be opened.
 */
XzraFile *XzraFileOpen(const char *filename);

/**
 * @brief Closes the given \c XzraFile. Does nothing if \p file is \c NULL.
 *
 * @param file File to close.
 * @return 0 on success, or
 * @return EOF on failure.
 */
int XzraFileClose(XzraFile *file);

/**
 * @brief Sets the file position indicator for the \c XzraFile to \p offset
 * uncompressed bytes relative to \p origin, which is either \c XZRA_SEEK_SET
 * (beginning of uncompressed file) or \c XZRA_SEEK_CUR (current indicator
 * position), and clears the EOF flag of \p file on success.
 *
 * @note This function does not check for EOF and therefore does not return an
 * error if \p offset is out of bounds. The EOF flag of \p file is also always
 * cleared on a successful call to this function without verifying the new
 * position.
 *
 * @param file Target file.
 * @param offset Offset relative to \p origin in uncompressed bytes.
 * @param origin if set to \c XZRA_SEEK_SET, the \p offset is relative to the
 * beginning of \p file; if set to \c XZRA_SEEK_CUR, the \p offset is relative
 * to the current indicator position.
 * @return 0 on success,
 * @return -1 on failure, which, in current implementation, can only happen if
 * \p origin is set to anything other than \c XZRA_SEEK_SET or
 * \c XZRA_SEEK_CUR.
 */
int XzraFileSeek(XzraFile *file, int64_t offset, int origin);

/**
 * @brief Reads \p size uncompressed bytes from the given \c XzraFile \p file
 * and stores the content into \p dest, which is assumed to have at least
 * \p size bytes. In case the EOF is reached before \p size bytes are read,
 * \c XzraFileRead will read as many bytes as possible, set the internal EOF
 * flag of the \c XzraFile to true, and return the number of bytes read. The
 * caller of this function is expected to either know the size of the remaining
 * uncompressed stream, or verify that the number of bytes returned is equal to
 * \p size to make sure that the read is successful.
 *
 * @param dest Destination buffer.
 * @param size Read size in uncompressed bytes.
 * @param file Source file.
 * @return Number of bytes read. In case the EOF is reached before \p size bytes
 * are read, \c XzraFileRead will read as many bytes as possible and return the
 * number of bytes read.
 */
size_t XzraFileRead(void *dest, size_t size, XzraFile *file);

/**
 * @brief Returns whether the end of the given \p file has been reached. While
 * the EOF flag is true, all future calls to \c XzraFileRead will do nothing and
 * return 0 until a successful call to \c XzraFileSeek is made on the same
 * \p file.
 *
 * @param file Target file.
 * @return true if the EOF of \p file has been reached,
 * @return false otherwise.
 */
bool XzraFileEOF(const XzraFile *file);

#endif  // GAMESMANONE_LIBS_XZRA_XZRA_H_

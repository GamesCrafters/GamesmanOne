/**
 * @file xzra.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief XZ utilities with random access.
 * @version 1.0.0
 * @date 2024-07-11
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

#ifndef GAMESMANONE_LIB_XZRA_XZRA_H_
#define GAMESMANONE_LIB_XZRA_XZRA_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // uint32_t, int64_t, uint64_t, uint8_t
#include <stdlib.h>   // size_t

// ============================== Compression API ==============================

/**
 * @brief Returns the memory usage (in bytes) of the XZRA compressor using the
 * given compression options.
 *
 * @param block_size Size of each uncompressed block.
 * @param level Compression level from 0 (store) to 9 (ultra).
 * @param extreme Extreme compression mode will be enabled if this parameter is
 * set to \c true. Enabling extreme compression slightly improves compression
 * ratio at the cost of increased compression time.
 * @param num_threads Number of threads to use for multithreaded compression.
 * The function will automatically detect and use the number of physical threads
 * available if this parameter is set to 0.
 * @return Number of bytes of memory required for compression using the given
 * options, or
 * @return UINT64_MAX if the given options are invalid.
 */
uint64_t XzraCompressionMemUsage(uint64_t block_size, uint32_t level,
                                 bool extreme, int num_threads);

/**
 * @brief Compresses input file of name \p ifname using a single LZMA2 filter
 * and stores the output XZ stream in output file of name \p ofname.
 *
 * @details The input file is first divided into blocks each of \p block_size
 * bytes, and then compressed in parallel using \p num_threads threads. Blocks
 * are independent of each other, thus allowing random access to the compresses
 * stream if \p block_size is a sufficiently small constant. The compression of
 * each block uses a dictionary of size equal to the size of the block to
 * minimize compressed size. Since the purpose of this library is to provide
 * fast random access to XZ files, using a large dictionary should be okay as we
 * are assuming \p block_size is small. Note that the setting of \p block_size
 * also affects compression ratio. In general, compression ratio deteriorates as
 * \p block_size decreases. XZ Utils enforces a minimum block size of 4 KiB and
 * recommends a minimum block size of 1 MiB for a reasonably good compression
 * ratio.
 *
 * @param ofname Output file name.
 * @param append The compressed XZ stream will be appened to the end of the
 * output file if this parameter is set to \c true.
 * @param block_size Size of each uncompressed block.
 * @param level Compression level from 0 (store) to 9 (ultra).
 * @param extreme Extreme compression mode will be enabled if this parameter is
 * set to \c true. Enabling extreme compression slightly improves compression
 * ratio at the cost of increased compression time.
 * @param num_threads Number of threads to use for multithreaded compression.
 * The function will automatically detect and use the number of physical threads
 * available if this parameter is set to 0.
 * @param ifname Input file name.
 * @return Size of the output file in bytes on success;
 * @return -1 if the input file cannot be opened;
 * @return -2 if the output file cannot be created or opened;
 * @return -3 if compression failed.
 */
int64_t XzraCompressFile(const char *ofname, bool append, uint64_t block_size,
                         uint32_t level, bool extreme, int num_threads,
                         const char *ifname);

/**
 * @brief Compresses input stream \p in of size \p in_size bytes using a single
 * LZMA2 filter and stores the output XZ stream in output file of name
 * \p ofname.
 *
 * @details The input stream is first divided into blocks each of \p block_size
 * bytes, and then compressed in parallel using \p num_threads threads. Blocks
 * are independent of each other, thus allowing random access to the compresses
 * stream if \p block_size is a sufficiently small constant. The compression of
 * each block uses a dictionary of size equal to the size of the block to
 * minimize compressed size. Since the purpose of this library is to provide
 * fast random access to XZ files, using a large dictionary should be okay as we
 * are assuming \p block_size is small. Note that the setting of \p block_size
 * also affects compression ratio. In general, compression ratio deteriorates as
 * \p block_size decreases. XZ Utils enforces a minimum block size of 4 KiB and
 * recommends a minimum block size of 1 MiB for a reasonably good compression
 * ratio.
 *
 * @param ofname Output file name.
 * @param append The compressed XZ stream will be appened to the end of the
 * output file if this parameter is set to \c true.
 * @param block_size Size of each uncompressed block.
 * @param level Compression level from 0 (store) to 9 (ultra).
 * @param extreme Extreme compression mode will be enabled if this parameter is
 * set to \c true. Enabling extreme compression slightly improves compression
 * ratio at the cost of increased compression time.
 * @param num_threads Number of threads to use for multithreaded compression.
 * The function will automatically detect and use the number of physical threads
 * available if this parameter is set to 0.
 * @param in Input stream.
 * @param in_size Size of the input stream in bytes.
 * @return Size of the output file in bytes on success;
 * @return -2 if the output file cannot be created or opened;
 * @return -3 if compression failed.
 */
int64_t XzraCompressStream(const char *ofname, bool append, uint64_t block_size,
                           uint32_t level, bool extreme, int num_threads,
                           uint8_t *in, size_t in_size);

// ============================= Decompression API =============================

/**
 * @brief Returns the memory usage (in bytes) of the XZRA decompressor.
 *
 * @param block_size Size of each uncompressed block. This is a property of the
 * compressed stream set at the time of compression.
 * @param level Compression level from 0 (store) to 9 (ultra). This is a
 * property of the compressed stream set at the time of compression.
 * @param extreme Extreme compression mode will be enabled if this parameter is
 * set to \c true. Enabling extreme compression slightly improves compression
 * ratio at the cost of increased compression time. This is a property of the
 * compressed stream set at the time of compression.
 * @param num_threads Number of threads to use for multithreaded decompression.
 * The function will automatically detect and use the number of physical threads
 * available if this parameter is set to 0.
 * @return Number of bytes of memory required for decompression using the given
 * options, or
 * @return UINT64_MAX if the given options are invalid.
 */
uint64_t XzraDecompressionMemUsage(uint64_t block_size, uint32_t level,
                                   bool extreme, int num_threads);

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
 * thread, the decompression will fail and -3 will be returned. Note that
 * \p memlimit does not include \p size, and it is the caller's responsibility
 * to take the output buffer size into account when calculating memory usage.
 *
 * @param dest Destination buffer, which is assumed to be of size at least
 * \p size bytes.
 * @param size Number of bytes to decompress.
 * @param num_threads Number of threads to use. The function will automatically
 * detect and use the number of physical threads available if this parameter is
 * set to 0.
 * @param memlimit Memory limit of the function in bytes. The function is
 * guaranteed not to allocate more than this amount of heap memory during its
 * execution. Setting this value too small may cause the function to use fewer
 * threads as specified depending on the property of the input file. If the
 * decompression cannot be completed under this memory limit even on a single
 * thread, the decompression will fail and -3 will be returned.
 * @param filename Name of the input file to decompress.
 * @return Number of bytes decompressed on success;
 * @return -1 if failed to initialize the XZ stream decoder;
 * @return -2 if failed to open file of name \p filename;
 * @return -3 if an error occurred during decompression;
 * @return -4 if failed to close the file.
 */
int64_t XzraDecompressFile(uint8_t *dest, size_t size, int num_threads,
                           uint64_t memlimit, const char *filename);

/** @brief Read-only XZ file with random access ability. */
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

#endif  // GAMESMANONE_LIB_XZRA_XZRA_H_

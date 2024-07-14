#ifndef LZ4_UTILS_H
#define LZ4_UTILS_H

#include <stddef.h>  // size_t
#include <stdint.h>  // int64_t

/**
 * @brief Compresses \p in_size bytes of \p in using level \p level LZ4 frame
 * compression, stores the compressed stream as file of name \p ofname, and
 * returns the size of the compressed file in bytes on success.
 *
 * @param in Input buffer.
 * @param in_size Size of the input buffer.
 * @param level LZ4 compression level.
 * @param ofname Output file name.
 * @return Size of the compressed file on success,
 * @return -1 if \p in is \c NULL and \p in_size is non-zero,
 * @return -2 if failed to allocate memory for compression, or
 * @return -3 if failed to create or write to the output file.
 */
int64_t Lz4UtilsCompressStream(const void* in, size_t in_size, int level,
                               const char* ofname);

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
int64_t Lz4UtilsCompressFile(const char* ifname, int level, const char* ofname);

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
 * buffer capacity.
 */
int64_t Lz4UtilsDecompressFile(const char* ifname, void* out, size_t out_size);

#endif  // LZ4_UTILS_H

/**
 * @file lz4_utils.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief LZ4 utilities implementation
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

#include "libs/lz4_utils/lz4_utils.h"

#include <assert.h>
#include <lz4frame.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// ================================= Constants =================================

// IO buffer size.
enum {
    kInChunkSize = 16 << 10  // 16 KiB
};

// A template for LZ4 compression preferences.
static const LZ4F_preferences_t kLz4PreferencesTemplate = {
    {
        // frameInfo
        LZ4F_max256KB,           // block size, using > LZ4 default 64KB
        LZ4F_blockLinked,        // use dependent blocks, improves compression
        LZ4F_noContentChecksum,  // frame checksum, disabled by default
        LZ4F_frame,              // frame type, using non-skippable frames
        0,                       // size of uncompressed content; 0 == unknown
        0,                       // dictionary ID; 0 == not provided
        LZ4F_noBlockChecksum,    // block checksum, disabled by default
    },
    0,          // compression level; 0 == default
    0,          // auto flush; 0 == disabled
    0,          // favor decompression speed for high compression mode; ignored
    {0, 0, 0},  // reserved, must be set to 0
};

// ========================== Common Helper Functions ==========================

static void *PointerShift(void *p, size_t n) { return (void *)((char *)p + n); }

static const void *PointerShiftConst(const void *p, size_t n) {
    return (const void *)((const char *)p + n);
}

// Returns p1 - p2.
static int64_t PointerDiff(const void *p1, const void *p2) {
    return (const char *)p1 - (const char *)p2;
}

static size_t SizeMin(size_t a, size_t b) { return a < b ? a : b; }

/**
 * @brief Same as LZ4F_isError except a message explaining the error is printed
 * if \p code is an error.
 */
static unsigned int Lz4fIsErrorExplained(LZ4F_errorCode_t code) {
    unsigned int ret = LZ4F_isError(code);
    if (ret) {
        fprintf(stderr, "error: %s (code %zu)\n", LZ4F_getErrorName(code),
                code);
    }

    return ret;
}

// ======================= Lz4UtilsCompressBuffersToFile =======================

static Lz4UtilsStatus CompressWriteFrameHeader(LZ4F_cctx *ctx,
                                               const LZ4F_preferences_t *pref,
                                               void *outbuf, size_t outbuf_size,
                                               FILE *f_out, size_t *count_out) {
    // Create compressed frame header in output buffer.
    const size_t header_size =
        LZ4F_compressBegin(ctx, outbuf, outbuf_size, pref);
    if (Lz4fIsErrorExplained(header_size)) {
        return LZ4_UTILS_ERR_COMPRESS;
    }

    // Write frame header to file.
    size_t written = fwrite(outbuf, 1, header_size, f_out);
    if (written != header_size) {
        return LZ4_UTILS_ERR_IO;
    }

    *count_out = header_size;
    return LZ4_UTILS_SUCCESS;
}

static Lz4UtilsStatus CompressFlushAndEnd(LZ4F_cctx *ctx, void *outbuf,
                                          size_t outbuf_size, FILE *f_out,
                                          size_t *count_out) {
    // Flush whatever remains within internal buffers.
    const size_t compressed_size =
        LZ4F_compressEnd(ctx, outbuf, outbuf_size, NULL);
    if (Lz4fIsErrorExplained(compressed_size)) {
        return LZ4_UTILS_ERR_COMPRESS;
    }

    // Write the remainder to file.
    size_t written = fwrite(outbuf, 1, compressed_size, f_out);
    if (written != compressed_size) {
        return LZ4_UTILS_ERR_IO;
    }

    *count_out += compressed_size;
    return LZ4_UTILS_SUCCESS;
}

static Lz4UtilsStatus CompressBuffersInternal(
    const void *const *in, const size_t *in_sizes, int n, FILE *f_out,
    LZ4F_cctx *ctx, const LZ4F_preferences_t *pref, size_t inbuf_size,
    void *outbuf, size_t outbuf_size, size_t *out_compressed_size) {
    // Write frame header
    size_t count_out = 0;
    Lz4UtilsStatus status = CompressWriteFrameHeader(
        ctx, pref, outbuf, outbuf_size, f_out, &count_out);
    if (status != LZ4_UTILS_SUCCESS) {
        return status;
    }

    // Stream compress all buffers to file.
    for (int i = 0; i < n; ++i) {
        size_t processed = 0;  // #Bytes processed from in[i]
        for (;;) {
            // We want to consume as much input as the output buffer allows.
            // Bounded by output buffer size: inbuf_size
            // Remaining in input buffer: in_sizes[i] - processed
            const size_t read_size =
                SizeMin(inbuf_size, in_sizes[i] - processed);
            if (read_size == 0) {
                break;  // Nothing left in input buffer.
            }

            // Compress to memory
            const void *next_in = PointerShiftConst(in[i], processed);
            size_t compressed_size = LZ4F_compressUpdate(
                ctx, outbuf, outbuf_size, next_in, read_size, NULL);
            if (Lz4fIsErrorExplained(compressed_size)) {
                return LZ4_UTILS_ERR_COMPRESS;
            }

            // Write output to file
            size_t written = fwrite(outbuf, 1, compressed_size, f_out);
            if (written != compressed_size) {
                return LZ4_UTILS_ERR_IO;
            }
            processed += read_size;
            count_out += compressed_size;
        }
    }

    // Flush the remaining bytes and end context.
    status = CompressFlushAndEnd(ctx, outbuf, outbuf_size, f_out, &count_out);
    if (status != LZ4_UTILS_SUCCESS) {
        return status;
    }

    // Report the number of compressed bytes if requested.
    if (out_compressed_size != NULL) {
        *out_compressed_size = count_out;
    }

    return LZ4_UTILS_SUCCESS;
}

static Lz4UtilsStatus CompressBuffers(const void *const *in,
                                      const size_t *in_sizes, int n, int level,
                                      FILE *f_out,
                                      size_t *out_compressed_size) {
    Lz4UtilsStatus status = LZ4_UTILS_SUCCESS;

    // Resource allocation.
    LZ4F_cctx *ctx;
    const LZ4F_errorCode_t ctx_creation =
        LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);  // may fail
    LZ4F_preferences_t preferences = kLz4PreferencesTemplate;
    preferences.compressionLevel = level;
    const size_t outbuf_size = LZ4F_compressBound(kInChunkSize, &preferences);
    void *const outbuf = malloc(outbuf_size);  // may fail
    if (Lz4fIsErrorExplained(ctx_creation) || outbuf == NULL) {
        status = LZ4_UTILS_ERR_OOM;
        goto _bailout;
    }

    status = CompressBuffersInternal(in, in_sizes, n, f_out, ctx, &preferences,
                                     kInChunkSize, outbuf, outbuf_size,
                                     out_compressed_size);

_bailout:
    free(outbuf);
    LZ4F_freeCompressionContext(ctx);  // supports free on NULL
    return status;
}

Lz4UtilsStatus Lz4UtilsCompressBuffersToFile(const void *const *in,
                                             const size_t *in_sizes, int n,
                                             int level, const char *ofname,
                                             size_t *out_compressed_size) {
    // Malformed input buffers array
    if (n > 0 && (in == NULL || in_sizes == NULL)) {
        return LZ4_UTILS_ERR_INVALID_PARAM;
    }

    // Input buffer is NULL while size is not 0
    for (int i = 0; i < n; ++i) {
        if (in_sizes[i] > 0 && in[i] == NULL) {
            return LZ4_UTILS_ERR_INVALID_PARAM;
        }
    }

    // Output file name is not given.
    if (ofname == NULL) {
        return LZ4_UTILS_ERR_INVALID_PARAM;
    }

    // Create output file or fail.
    FILE *const f_out = fopen(ofname, "wb");
    if (f_out == NULL) {
        return LZ4_UTILS_ERR_IO;
    }

    const Lz4UtilsStatus status =
        CompressBuffers(in, in_sizes, n, level, f_out, out_compressed_size);
    fclose(f_out);

    return status;
}

// ======================= Lz4UtilsCompressBufferToFile =======================

Lz4UtilsStatus Lz4UtilsCompressBufferToFile(const void *in, size_t in_size,
                                            int level, const char *ofname,
                                            size_t *out_compressed_size) {
    const void *inputs[1] = {in};
    const size_t input_sizes[1] = {in_size};

    return Lz4UtilsCompressBuffersToFile(inputs, input_sizes, 1, level, ofname,
                                         out_compressed_size);
}

// ======================== Lz4UtilsCompressFileToFile ========================

static Lz4UtilsStatus CompressFileInternal(FILE *f_in, FILE *f_out,
                                           LZ4F_cctx *ctx,
                                           const LZ4F_preferences_t *pref,
                                           void *inbuf, size_t inbuf_size,
                                           void *outbuf, size_t outbuf_size,
                                           size_t *out_compressed_size) {
    // Write frame header
    size_t count_out = 0;
    Lz4UtilsStatus status = CompressWriteFrameHeader(
        ctx, pref, outbuf, outbuf_size, f_out, &count_out);
    if (status != LZ4_UTILS_SUCCESS) {
        return status;
    }

    // Stream file.
    for (;;) {
        // Read as much from the input file as allowed by input buffer size.
        const size_t read_size = fread(inbuf, 1, inbuf_size, f_in);
        if (read_size == 0) {
            break;  // Nothing left to read from input file.
        }

        // Compress to memory
        size_t compressed_size = LZ4F_compressUpdate(ctx, outbuf, outbuf_size,
                                                     inbuf, read_size, NULL);
        if (Lz4fIsErrorExplained(compressed_size)) {
            return LZ4_UTILS_ERR_COMPRESS;
        }

        // Write output to file
        size_t written = fwrite(outbuf, 1, compressed_size, f_out);
        if (written != compressed_size) {
            return LZ4_UTILS_ERR_IO;
        }

        count_out += compressed_size;
    }

    // Flush the remaining bytes and end context.
    status = CompressFlushAndEnd(ctx, outbuf, outbuf_size, f_out, &count_out);
    if (status != LZ4_UTILS_SUCCESS) {
        return status;
    }

    // Report the number of compressed bytes if requested.
    if (out_compressed_size != NULL) {
        *out_compressed_size = count_out;
    }

    return LZ4_UTILS_SUCCESS;
}

static Lz4UtilsStatus CompressFile(FILE *f_in, int level, FILE *f_out,
                                   size_t *out_compressed_size) {
    Lz4UtilsStatus status = LZ4_UTILS_SUCCESS;

    // Resource allocation.
    LZ4F_cctx *ctx;
    const LZ4F_errorCode_t ctx_creation =
        LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);  // may fail
    char inbuf[kInChunkSize];
    LZ4F_preferences_t preferences = kLz4PreferencesTemplate;
    preferences.compressionLevel = level;
    const size_t outbuf_size = LZ4F_compressBound(kInChunkSize, &preferences);
    void *const outbuf = malloc(outbuf_size);  // may fail
    if (Lz4fIsErrorExplained(ctx_creation) || outbuf == NULL) {
        status = LZ4_UTILS_ERR_OOM;
        goto _bailout;
    }

    status = CompressFileInternal(f_in, f_out, ctx, &preferences, inbuf,
                                  kInChunkSize, outbuf, outbuf_size,
                                  out_compressed_size);

_bailout:
    LZ4F_freeCompressionContext(ctx); /* supports free on NULL */
    free(outbuf);
    return status;
}

Lz4UtilsStatus Lz4UtilsCompressFileToFile(const char *ifname, int level,
                                          const char *ofname,
                                          size_t *out_compressed_size) {
    // File names must not be NULL
    if (ifname == NULL || ofname == NULL) {
        return LZ4_UTILS_ERR_INVALID_PARAM;
    }

    // Open input file or fail.
    FILE *const f_in = fopen(ifname, "rb");
    if (f_in == NULL) {
        return LZ4_UTILS_ERR_IO;
    }

    // Create output file or fail.
    FILE *const f_out = fopen(ofname, "wb");
    if (f_out == NULL) {
        fclose(f_in);
        return LZ4_UTILS_ERR_IO;
    }

    const Lz4UtilsStatus status =
        CompressFile(f_in, level, f_out, out_compressed_size);
    fclose(f_in);
    fclose(f_out);

    return status;
}

// ============================= Lz4UtilsOutStream =============================

struct Lz4UtilsOutStream {
    FILE *f_out;                    /**< Output file */
    LZ4F_cctx *ctx;                 /**< LZ4F context */
    LZ4F_preferences_t preferences; /**< LZ4F preferences */
    size_t bufsize;                 /**< Output file */
    size_t compressed_size;         /**< #Bytes compressed */
    char buf[];                     /**< Output buffer for compressed data */
};

// ========================== Lz4UtilsOutStreamCreate ==========================

Lz4UtilsOutStream *Lz4UtilsOutStreamCreate(const char *ofname, int level) {
    if (ofname == NULL) {
        return NULL;
    }

    // Calculate output buffer size
    LZ4F_preferences_t preferences = kLz4PreferencesTemplate;
    preferences.compressionLevel = level;
    size_t bufsize = LZ4F_compressBound(kInChunkSize, &preferences);
    Lz4UtilsOutStream *ret =
        (Lz4UtilsOutStream *)calloc(1, sizeof(Lz4UtilsOutStream) + bufsize);
    if (!ret) {
        return NULL;
    }

    ret->preferences = preferences;
    ret->bufsize = bufsize;

    // Resources are allocated, we rely on goto to clean up on failure
    bool success = true;

    // Create/open output file
    ret->f_out = fopen(ofname, "wb");
    if (ret->f_out == NULL) {
        fprintf(stderr,
                "Lz4UtilsOutStreamCreate: failed to open output file %s\n",
                ofname);
        success = false;
        goto _bailout;
    }

    // Create LZ4F context
    const LZ4F_errorCode_t ctx_creation =
        LZ4F_createCompressionContext(&ret->ctx, LZ4F_VERSION);
    if (Lz4fIsErrorExplained(ctx_creation)) {
        success = false;
        goto _bailout;
    }

    // Compress frame header into output buffer
    const size_t header_size =
        LZ4F_compressBegin(ret->ctx, ret->buf, ret->bufsize, &ret->preferences);
    if (Lz4fIsErrorExplained(header_size)) {
        success = false;
        goto _bailout;
    }

    // Write compressed file header to file
    size_t written = fwrite(ret->buf, 1, header_size, ret->f_out);
    ret->compressed_size += written;
    if (written != header_size) {
        success = false;
        goto _bailout;
    }

_bailout:
    if (!success) {
        if (ret->f_out) {
            fclose(ret->f_out);
            ret->f_out = NULL;
        }
        LZ4F_freeCompressionContext(ret->ctx);
        free(ret);
        ret = NULL;
    }

    return ret;
}

// =========================== Lz4UtilsOutStreamRun ===========================

Lz4UtilsStatus Lz4UtilsOutStreamRun(Lz4UtilsOutStream *stream, const void *in,
                                    size_t in_size, size_t *out_bytes_written) {
    if (!stream || (!in && in_size > 0)) {
        return LZ4_UTILS_ERR_INVALID_PARAM;
    }

    size_t begin_outsize = stream->compressed_size;
    size_t processed = 0;
    while (processed < in_size) {
        // Compress at most kInChunkSize bytes or the rest of remaining bytes
        // left in the input buffer.
        const size_t read_size = SizeMin(kInChunkSize, in_size - processed);
        const void *next_in = PointerShiftConst(in, processed);
        size_t compressed_size =
            LZ4F_compressUpdate(stream->ctx, stream->buf, stream->bufsize,
                                next_in, read_size, NULL);
        if (Lz4fIsErrorExplained(compressed_size)) {
            return LZ4_UTILS_ERR_COMPRESS;
        }

        // Write compressed data to file
        size_t written = fwrite(stream->buf, 1, compressed_size, stream->f_out);
        if (written != compressed_size) {
            return LZ4_UTILS_ERR_IO;
        }

        processed += read_size;
        stream->compressed_size += compressed_size;
    }

    if (out_bytes_written != NULL) {
        *out_bytes_written = stream->compressed_size - begin_outsize;
    }
    return LZ4_UTILS_SUCCESS;
}

// ========================== Lz4UtilsOutStreamClose ==========================

Lz4UtilsStatus Lz4UtilsOutStreamClose(Lz4UtilsOutStream *stream,
                                      size_t *out_total_compressed_size) {
    if (stream == NULL) {
        return LZ4_UTILS_SUCCESS;
    }

    Lz4UtilsStatus status = LZ4_UTILS_SUCCESS;

    // Flush whatever remains within internal buffers
    const size_t compressed_size =
        LZ4F_compressEnd(stream->ctx, stream->buf, stream->bufsize, NULL);
    if (Lz4fIsErrorExplained(compressed_size)) {
        status = LZ4_UTILS_ERR_COMPRESS;
        goto _bailout;
    }

    // Write remaining compressed data to file
    size_t written = fwrite(stream->buf, 1, compressed_size, stream->f_out);
    if (written != compressed_size) {
        status = LZ4_UTILS_ERR_IO;
        goto _bailout;
    }

    // Update compressed size and report it if requested
    stream->compressed_size += compressed_size;
    if (out_total_compressed_size != NULL) {
        *out_total_compressed_size = stream->compressed_size;
    }

_bailout:
    // Unconditional cleanup
    fclose(stream->f_out);
    LZ4F_freeCompressionContext(stream->ctx);
    free(stream);

    return status;
}

// ===================== Lz4UtilsDecompressFileToBuffers =====================

// LZ4F_decompress has a somewhat confusing API. The 3rd
// (dstSizePtr) and the 5th (srcSizePtr) parameters are used as
// input and output parameters at the same time. They initially hold
// the CAPACITIES of the buffers and are set to the number of
// ACTUALLY CONSUMED bytes after the function call.
// Note that src is the read buffer for compressed file contents from f_in.
static Lz4UtilsStatus DecompressFileInternal(FILE *f_in, void *const *out,
                                             const size_t *out_sizes, int n,
                                             LZ4F_dctx *dctx, void *src,
                                             size_t src_capacity,
                                             size_t *out_uncompressed_size) {
    size_t total_size = 0;  // Total number of uncompressed bytes generated
    size_t lz4f_code = 1;   // LZ4F return value
    size_t out_offset = 0;  // Byte offset in the current output buffer
    int out_index = 0;      // Index of the current output buffer

    // Loop until an LZ4 frame is fully decompressed (lz4f_code == 0)
    while (lz4f_code != 0) {
        // Fill the input buffer
        size_t read_size = fread(src, 1, src_capacity, f_in);

        // EOF reached before end-of-frame indicates file corruption
        if (read_size == 0 && feof(f_in)) {
            return LZ4_UTILS_ERR_CORRUPT_DATA;
        }

        // Check for read errors
        if (ferror(f_in)) {
            return LZ4_UTILS_ERR_IO;
        }

        // Address in the input buffer where decompression should begin
        const void *src_begin = src;

        // Address in the input buffer where decompression should stop
        const void *const src_end = (const char *)src + read_size;

        // Process the buffered input until exhausted OR the frame completes
        while (src_begin < src_end && lz4f_code != 0) {
            // Advance to the next output buffer if the current one is full.
            // A while loop is used in case there are 0-size buffers in the
            // array.
            while (out_index < n && out_offset >= out_sizes[out_index]) {
                out_index++;
                out_offset = 0;
            }

            void *out_begin = NULL;
            size_t dest_buffer_size = 0;

            // If we still have valid output capacity, set up the destination
            // buffer
            if (out_index < n) {
                out_begin = PointerShift(out[out_index], out_offset);
                dest_buffer_size = out_sizes[out_index] - out_offset;
            }

            size_t src_buffer_size = PointerDiff(src_end, src_begin);

            // Allow decompression to proceed. If dest_buffer_size is 0,
            // LZ4F_decompress will just consume input (like headers) until it
            // actually needs to emit data.
            lz4f_code = LZ4F_decompress(dctx, out_begin, &dest_buffer_size,
                                        src_begin, &src_buffer_size, NULL);

            if (Lz4fIsErrorExplained(lz4f_code)) {
                return LZ4_UTILS_ERR_CORRUPT_DATA;
            }

            // If the decompressor couldn't consume any input AND couldn't
            // produce output, it is stalled because it has actual data to
            // output but no buffer space.
            if (src_buffer_size == 0 && dest_buffer_size == 0) {
                return LZ4_UTILS_ERR_INSUFFICIENT_BUF;
            }

            out_offset += dest_buffer_size;
            total_size += dest_buffer_size;
            src_begin = PointerShiftConst(src_begin, src_buffer_size);
        }
    }

    // Report the number of decompressed bytes if requested
    if (out_uncompressed_size != NULL) {
        *out_uncompressed_size = total_size;
    }

    return LZ4_UTILS_SUCCESS;
}

static Lz4UtilsStatus DecompressFileMultistream(FILE *f_in, void *const *out,
                                                const size_t *out_sizes, int n,
                                                size_t *out_uncompressed_size) {
    // Create decompression context
    LZ4F_dctx *dctx;
    size_t const dctx_status =
        LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
    if (Lz4fIsErrorExplained(dctx_status)) {
        return LZ4_UTILS_ERR_OOM;
    }

    // Run the internal decompression logic
    char src[kInChunkSize];
    const Lz4UtilsStatus status =
        DecompressFileInternal(f_in, out, out_sizes, n, dctx, src, kInChunkSize,
                               out_uncompressed_size);

    LZ4F_errorCode_t code = LZ4F_freeDecompressionContext(dctx);
    if (Lz4fIsErrorExplained(code)) {
        fprintf(
            stderr,
            "DecompressFileMultistream: (warning) decompression was incomplete "
            "when context was freed\n");
    }
    return status;
}

Lz4UtilsStatus Lz4UtilsDecompressFileToBuffers(const char *ifname,
                                               void *const *out,
                                               const size_t *out_sizes, int n,
                                               size_t *out_uncompressed_size) {
    // A negative number of buffers a bug
    if (n < 0) {
        return LZ4_UTILS_ERR_INVALID_PARAM;
    }

    // Nothing to decompress
    if (n == 0) {
        return LZ4_UTILS_SUCCESS;
    }

    // Validate the input file name, output buffers array and sizes array
    if (!ifname || !out || !out_sizes) {
        return LZ4_UTILS_ERR_INVALID_PARAM;
    }
    for (int i = 0; i < n; ++i) {
        // A NULL buffer with a positive size is a bug
        if (out_sizes[i] > 0 && out[i] == NULL) {
            return LZ4_UTILS_ERR_INVALID_PARAM;
        }
    }

    // Open the input file or fail
    FILE *const f_in = fopen(ifname, "rb");
    if (!f_in) {
        return LZ4_UTILS_ERR_IO;
    }

    // Run the decompressor
    const Lz4UtilsStatus status = DecompressFileMultistream(
        f_in, out, out_sizes, n, out_uncompressed_size);
    fclose(f_in);

    return status;
}

// ====================== Lz4UtilsDecompressFileToBuffer ======================

Lz4UtilsStatus Lz4UtilsDecompressFileToBuffer(const char *ifname, void *out,
                                              size_t out_size,
                                              size_t *out_uncompressed_size) {
    void *out_buffers[1] = {out};
    const size_t out_sizes[1] = {out_size};

    return Lz4UtilsDecompressFileToBuffers(ifname, out_buffers, out_sizes, 1,
                                           out_uncompressed_size);
}

// ============================= Lz4UtilsInStream =============================

struct Lz4UtilsInStream {
    FILE *f_in;             /**< Input (compressed) file */
    LZ4F_dctx *dctx;        /**< Decompression context */
    size_t lz4f_code;       /**< LZ4F return value */
    size_t buf_size;        /**< Number of compressed bytes available in buf */
    size_t buf_offset;      /**< Number of bytes already consumed from buf */
    char buf[kInChunkSize]; /**< Buffer for compressed data */
};

// ========================== Lz4UtilsInStreamCreate ==========================

Lz4UtilsInStream *Lz4UtilsInStreamCreate(const char *ifname) {
    if (ifname == NULL) {
        return NULL;
    }

    // Allocate memory or fail
    Lz4UtilsInStream *ret =
        (Lz4UtilsInStream *)calloc(1, sizeof(Lz4UtilsInStream));
    if (!ret) {
        return NULL;
    }

    // Open input file or fail
    ret->f_in = fopen(ifname, "rb");
    if (ret->f_in == NULL) {
        free(ret);
        return NULL;
    }

    // Create decompression context or fail
    size_t const dctx_status =
        LZ4F_createDecompressionContext(&ret->dctx, LZ4F_VERSION);
    if (Lz4fIsErrorExplained(dctx_status)) {
        fclose(ret->f_in);
        free(ret);
        return NULL;
    }

    ret->lz4f_code = 1;

    return ret;
}

// ============================ Lz4UtilsInStreamRun ============================

Lz4UtilsStatus Lz4UtilsInStreamRun(Lz4UtilsInStream *stream, void *out,
                                   size_t out_size,
                                   size_t *out_uncompressed_size) {
    if (stream == NULL || out == NULL) {
        return LZ4_UTILS_ERR_INVALID_PARAM;
    }

    // Keep a record of how many bytes we decompressed before this run
    size_t initial_out_size = out_size;

    // Continue while the frame isn't finished and we have output capacity
    while (stream->lz4f_code != 0 && out_size > 0) {
        // Refill input buffer if exhausted
        if (stream->buf_offset >= stream->buf_size) {
            stream->buf_offset = 0;
            stream->buf_size =
                fread(stream->buf, 1, kInChunkSize, stream->f_in);

            // Check for I/O error
            if (ferror(stream->f_in)) {
                return LZ4_UTILS_ERR_IO;
            }

            // Check for unexpected EOF
            if (stream->buf_size == 0) {
                return LZ4_UTILS_ERR_CORRUPT_DATA;
            }
        }

        // Run decompression
        size_t dest_buffer_size = out_size;
        size_t src_buffer_size = stream->buf_size - stream->buf_offset;
        const void *src_begin =
            PointerShiftConst(stream->buf, stream->buf_offset);
        stream->lz4f_code =
            LZ4F_decompress(stream->dctx, out, &dest_buffer_size, src_begin,
                            &src_buffer_size, NULL);
        // After LZ4F_decompress, dest_buffer_size becomes bytes written to out,
        // and src_buffer_size becomes bytes consumed from stream->buf

        if (Lz4fIsErrorExplained(stream->lz4f_code)) {
            return LZ4_UTILS_ERR_CORRUPT_DATA;
        }

        // Update stream internal states
        out = PointerShift(out, dest_buffer_size);
        out_size -= dest_buffer_size;
        stream->buf_offset += src_buffer_size;
    }

    // Report the number of bytes decompressed if requested
    if (out_uncompressed_size != NULL) {
        *out_uncompressed_size = initial_out_size - out_size;
    }

    return LZ4_UTILS_SUCCESS;
}

// =========================== Lz4UtilsInStreamClose ===========================

void Lz4UtilsInStreamClose(Lz4UtilsInStream *stream) {
    if (stream == NULL) {
        return;
    }

    fclose(stream->f_in);
    LZ4F_errorCode_t code = LZ4F_freeDecompressionContext(stream->dctx);
    if (Lz4fIsErrorExplained(code)) {
        fprintf(stderr,
                "Lz4UtilsInStreamClose: (warning) decompression was incomplete "
                "when context was freed\n");
    }
    free(stream);
}

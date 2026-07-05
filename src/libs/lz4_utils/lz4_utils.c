/**
 * @file lz4_utils.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief LZ4 utilities implementation
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

#include "libs/lz4_utils/lz4_utils.h"

#include <lz4frame.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// ================================= Constants =================================

// IO buffer size.
static const int kInChunkSize = 16 << 10;  // 16 KiB

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

static void *GenericPointerShift(const void *p, size_t n) {
    return (void *)((char *)p + n);
}

// Returns p1 - p2.
static int64_t GenericPointerDiff(const void *p1, const void *p2) {
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

// ========================== Lz4UtilsCompressBuffersToFile
// ==========================

static int64_t CompressStreamsInternal(const void *const *in,
                                       const size_t *in_sizes, int n,
                                       FILE *f_out, LZ4F_cctx *ctx,
                                       const LZ4F_preferences_t *pref,
                                       size_t inbuf_size, void *outbuf,
                                       size_t outbuf_size) {
    // Write frame header.
    const size_t header_size =
        LZ4F_compressBegin(ctx, outbuf, outbuf_size, pref);
    if (Lz4fIsErrorExplained(header_size)) return -2;

    size_t count_out = header_size;
    size_t written = fwrite(outbuf, 1, header_size, f_out);
    if (written != header_size) return -3;

    // Stream file.
    for (int i = 0; i < n; ++i) {
        size_t processed = 0;  // #Bytes processed from in[i]
        for (;;) {
            const size_t read_size =
                SizeMin(inbuf_size, in_sizes[i] - processed);
            if (read_size == 0) break;  // Nothing left in input buffer.
            const void *next_in = GenericPointerShift(in[i], processed);
            size_t compressed_size = LZ4F_compressUpdate(
                ctx, outbuf, outbuf_size, next_in, read_size, NULL);
            if (Lz4fIsErrorExplained(compressed_size)) return -2;
            written = fwrite(outbuf, 1, compressed_size, f_out);
            if (written != compressed_size) return -3;
            processed += read_size;
            count_out += compressed_size;
        }
    }

    // Flush whatever remains within internal buffers.
    const size_t compressed_size =
        LZ4F_compressEnd(ctx, outbuf, outbuf_size, NULL);
    if (Lz4fIsErrorExplained(compressed_size)) return -2;
    written = fwrite(outbuf, 1, compressed_size, f_out);
    if (written != compressed_size) return -3;
    count_out += compressed_size;

    return (int64_t)count_out;
}

static int64_t CompressStreams(const void *const *in, const size_t *in_sizes,
                               int n, int level, FILE *f_out) {
    int64_t ret;

    // Resource allocation.
    LZ4F_cctx *ctx;
    const LZ4F_errorCode_t ctx_creation =
        LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);  // may fail
    LZ4F_preferences_t preferences = kLz4PreferencesTemplate;
    preferences.compressionLevel = level;
    const size_t outbuf_size = LZ4F_compressBound(kInChunkSize, &preferences);
    void *const outbuf = malloc(outbuf_size);  // may fail
    if (Lz4fIsErrorExplained(ctx_creation) || outbuf == NULL) {
        ret = -1;
        goto _bailout;
    }

    ret = CompressStreamsInternal(in, in_sizes, n, f_out, ctx, &preferences,
                                  kInChunkSize, outbuf, outbuf_size);

_bailout:
    LZ4F_freeCompressionContext(ctx); /* supports free on NULL */
    free(outbuf);
    return ret;
}

int64_t Lz4UtilsCompressBuffersToFile(const void *const *in,
                                      const size_t *in_sizes, int n, int level,
                                      const char *ofname) {
    if (n > 0 && (in == NULL || in_sizes == NULL)) return -1;
    for (int i = 0; i < n; ++i) {
        if (in_sizes[i] > 0 && in[i] == NULL) return -1;
    }

    if (ofname == NULL) return -3;
    FILE *const f_out = fopen(ofname, "wb");
    if (f_out == NULL) return -3;

    const int64_t ret = CompressStreams(in, in_sizes, n, level, f_out);
    fclose(f_out);

    return ret;
}

// ========================== Lz4UtilsCompressBufferToFile
// ==========================

int64_t Lz4UtilsCompressBufferToFile(const void *in, size_t in_size, int level,
                                     const char *ofname) {
    const void *inputs[] = {in};
    const size_t input_sizes[] = {in_size};

    return Lz4UtilsCompressBuffersToFile(inputs, input_sizes, 1, level, ofname);
}

// =========================== Lz4UtilsCompressFileToFile
// ===========================

static int64_t CompressFileInternal(FILE *f_in, FILE *f_out, LZ4F_cctx *ctx,
                                    const LZ4F_preferences_t *pref, void *inbuf,
                                    size_t inbuf_size, void *outbuf,
                                    size_t outbuf_size) {
    // Write frame header.
    const size_t header_size =
        LZ4F_compressBegin(ctx, outbuf, outbuf_size, pref);
    if (Lz4fIsErrorExplained(header_size)) return -2;

    size_t count_out = header_size;
    size_t n = fwrite(outbuf, 1, header_size, f_out);
    if (n != header_size) return -3;

    // Stream file.
    for (;;) {
        const size_t read_size = fread(inbuf, 1, inbuf_size, f_in);
        if (read_size == 0) break;  // Nothing left to read from input file.
        size_t compressed_size = LZ4F_compressUpdate(ctx, outbuf, outbuf_size,
                                                     inbuf, read_size, NULL);
        if (Lz4fIsErrorExplained(compressed_size)) return -2;
        n = fwrite(outbuf, 1, compressed_size, f_out);
        if (n != compressed_size) return -3;
        count_out += compressed_size;
    }

    // Flush whatever remains within internal buffers.
    const size_t compressed_size =
        LZ4F_compressEnd(ctx, outbuf, outbuf_size, NULL);
    if (Lz4fIsErrorExplained(compressed_size)) return -2;
    n = fwrite(outbuf, 1, compressed_size, f_out);
    if (n != compressed_size) return -3;
    count_out += compressed_size;

    return (int64_t)count_out;
}

static int64_t CompressFile(FILE *f_in, int level, FILE *f_out) {
    int64_t ret;

    // Resource allocation.
    LZ4F_cctx *ctx;
    const LZ4F_errorCode_t ctx_creation =
        LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);  // may fail
    void *const inbuf = malloc(kInChunkSize);               // may fail
    LZ4F_preferences_t preferences = kLz4PreferencesTemplate;
    preferences.compressionLevel = level;
    const size_t outbuf_size = LZ4F_compressBound(kInChunkSize, &preferences);
    void *const outbuf = malloc(outbuf_size);  // may fail
    if (Lz4fIsErrorExplained(ctx_creation) || inbuf == NULL || outbuf == NULL) {
        ret = -1;
        goto _bailout;
    }

    ret = CompressFileInternal(f_in, f_out, ctx, &preferences, inbuf,
                               kInChunkSize, outbuf, outbuf_size);

_bailout:
    LZ4F_freeCompressionContext(ctx); /* supports free on NULL */
    free(inbuf);
    free(outbuf);
    return ret;
}

int64_t Lz4UtilsCompressFileToFile(const char *ifname, int level,
                                   const char *ofname) {
    FILE *const f_in = fopen(ifname, "rb");
    if (f_in == NULL) return -1;

    FILE *const f_out = fopen(ofname, "wb");
    if (f_out == NULL) {
        fclose(f_in);
        return -3;
    }

    const int64_t ret = CompressFile(f_in, level, f_out);
    fclose(f_in);
    fclose(f_out);

    return ret;
}

// ============================= Lz4UtilsOutStream =============================

struct Lz4UtilsOutStream {
    FILE *f_out; /**< Output file. */
    LZ4F_preferences_t preferences;
    LZ4F_cctx *ctx;
    void *buf;
    size_t bufsize;
    int64_t outsize;
};

// ========================== Lz4UtilsOutStreamCreate ==========================

Lz4UtilsOutStream *Lz4UtilsOutStreamCreate(const char *ofname, int level) {
    bool success = true;
    if (ofname == NULL) return NULL;

    Lz4UtilsOutStream *ret =
        (Lz4UtilsOutStream *)calloc(1, sizeof(Lz4UtilsOutStream));
    if (!ret) return NULL;

    ret->f_out = fopen(ofname, "wb");
    if (ret->f_out == NULL) {
        fprintf(stderr,
                "Lz4UtilsOutStreamCreate: failed to open output file %s\n",
                ofname);
        success = false;
        goto _bailout;
    }

    const LZ4F_errorCode_t ctx_creation =
        LZ4F_createCompressionContext(&ret->ctx, LZ4F_VERSION);
    ret->preferences = kLz4PreferencesTemplate;
    ret->preferences.compressionLevel = level;
    ret->bufsize = LZ4F_compressBound(kInChunkSize, &ret->preferences);
    ret->buf = malloc(ret->bufsize);
    if (Lz4fIsErrorExplained(ctx_creation) || ret->buf == NULL) {
        success = false;
        goto _bailout;
    }

    // Write frame header.
    const size_t header_size =
        LZ4F_compressBegin(ret->ctx, ret->buf, ret->bufsize, &ret->preferences);
    if (Lz4fIsErrorExplained(header_size)) {
        success = false;
        goto _bailout;
    }

    ret->outsize += header_size;
    size_t written = fwrite(ret->buf, 1, header_size, ret->f_out);
    if (written != header_size) {
        success = false;
        goto _bailout;
    }

_bailout:
    if (!success) {
        free(ret->buf);
        LZ4F_freeCompressionContext(ret->ctx); /* supports free on NULL */
        if (ret->f_out) fclose(ret->f_out);
        free(ret);
        ret = NULL;
    }

    return ret;
}

// =========================== Lz4UtilsOutStreamRun ===========================

int64_t Lz4UtilsOutStreamRun(Lz4UtilsOutStream *stream, const void *in,
                             size_t in_size) {
    int64_t begin_outsize = stream->outsize;
    size_t processed = 0;
    while (processed < in_size) {
        // Process at most kInChunkSize bytes or the rest of remaining bytes
        // left in the input buffer.
        const size_t read_size = SizeMin(kInChunkSize, in_size - processed);
        const void *next_in = GenericPointerShift(in, processed);
        size_t compressed_size =
            LZ4F_compressUpdate(stream->ctx, stream->buf, stream->bufsize,
                                next_in, read_size, NULL);
        if (Lz4fIsErrorExplained(compressed_size)) return -2;
        size_t written = fwrite(stream->buf, 1, compressed_size, stream->f_out);
        if (written != compressed_size) return -3;
        processed += read_size;
        stream->outsize += compressed_size;
    }

    return stream->outsize - begin_outsize;
}

// ========================== Lz4UtilsOutStreamClose ==========================

int64_t Lz4UtilsOutStreamClose(Lz4UtilsOutStream *stream) {
    if (stream == NULL) return 0;

    int64_t ret = 0;

    // Flush whatever remains within internal buffers.
    const size_t compressed_size =
        LZ4F_compressEnd(stream->ctx, stream->buf, stream->bufsize, NULL);

    if (Lz4fIsErrorExplained(compressed_size)) {
        ret = -2;
    } else {
        size_t written = fwrite(stream->buf, 1, compressed_size, stream->f_out);
        if (written != compressed_size) {
            ret = -3;
        } else {
            stream->outsize += compressed_size;
            ret = stream->outsize;
        }
    }

    // Unconditional cleanup
    free(stream->buf);
    LZ4F_freeCompressionContext(stream->ctx);
    fclose(stream->f_out);
    free(stream);

    return ret;
}

// ===================== Lz4UtilsDecompressFileToBuffers =====================

// LZ4F_decompress has a somewhat confusing API. The 3rd
// (dstSizePtr) and the 5th (srcSizePtr) parameters are used as
// input and output parameters at the same time. They initially hold
// the CAPACITIES of the buffers and are set to the number of
// ACTUALLY CONSUMED bytes after the function call.
// Note that src is the read buffer for compressed file contents from f_in.
static int64_t DecompressFileInternal(FILE *f_in, void **out,
                                      const size_t *out_sizes, int n,
                                      LZ4F_dctx *dctx, void *src,
                                      size_t src_capacity) {
    int64_t total_size = 0;
    size_t lz4f_code = 1;
    int out_index;
    size_t out_offset = 0;
    for (out_index = 0; out_index < n; ++out_index) {
        if (out_sizes[out_index] != 0) break;
    }
    if (out_index == n) return total_size;

    // While in this loop, there are more compressed contents in f_in to process
    while (lz4f_code != 0) {
        size_t read_size =
            fread(src, 1, src_capacity, f_in);  // Load more input
        const void *src_begin = src;
        const void *const src_end = (const char *)src + read_size;
        if (read_size == 0 || ferror(f_in)) return -3;

        // While in this loop, there is more input in buffer and the frame isn't
        // over.
        while (src_begin < src_end && lz4f_code != 0) {
            if (out_index >= n) {
                // Output buffers exhausted, but frame data remains.
                // Prevent infinite loop by returning an error code.
                return -5;
            }

            // While in this loop, the contents in src have not been fully
            // consumed and we still have at least one output buffer to fill.
            while (out_index < n) {
                void *out_begin =
                    GenericPointerShift(out[out_index], out_offset);
                size_t dest_buffer_size = out_sizes[out_index] - out_offset;
                size_t src_buffer_size = GenericPointerDiff(src_end, src_begin);
                lz4f_code = LZ4F_decompress(dctx, out_begin, &dest_buffer_size,
                                            src_begin, &src_buffer_size, NULL);
                if (Lz4fIsErrorExplained(lz4f_code)) return -4;
                out_offset += dest_buffer_size;
                total_size += (int64_t)dest_buffer_size;
                src_begin = GenericPointerShift(src_begin, src_buffer_size);

                // Break if all bytes in src are fully consumed for this round
                if (src_begin >= src_end) break;

                // Otherwise, we move on to the next output buffer
                out_offset = 0;
                do {
                    ++out_index;
                } while (out_index < n && out_sizes[out_index] == 0);
            }
        }
    }

    return total_size;
}

static int64_t DecompressFileMultistream(FILE *f_in, void **out,
                                         const size_t *out_sizes, int n) {
    // Resource allocation.
    void *const src = malloc(kInChunkSize);
    if (src == NULL) return -2;

    LZ4F_dctx *dctx;
    size_t const dctx_status =
        LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
    if (Lz4fIsErrorExplained(dctx_status)) {
        free(src);
        return -2;
    }

    int64_t const result = DecompressFileInternal(f_in, out, out_sizes, n, dctx,
                                                  src, kInChunkSize);

    free(src);
    LZ4F_freeDecompressionContext(dctx);
    return result;
}

int64_t Lz4UtilsDecompressFileToBuffers(const char *ifname, void **out,
                                        const size_t *out_sizes, int n) {
    if (n > 0 && (out == NULL || out_sizes == NULL)) return -4;
    for (int i = 0; i < n; ++i) {
        if (out_sizes[i] > 0 && out[i] == NULL) return -4;
    }

    if (ifname == NULL) return -1;
    FILE *const f_in = fopen(ifname, "rb");
    if (f_in == NULL) return -1;

    const int64_t ret = DecompressFileMultistream(f_in, out, out_sizes, n);
    fclose(f_in);

    return ret;
}

// ========================== Lz4UtilsDecompressFileToBuffer
// ==========================

int64_t Lz4UtilsDecompressFileToBuffer(const char *ifname, void *out,
                                       size_t out_size) {
    void *out_buffers[] = {out};
    const size_t out_sizes[] = {out_size};

    return Lz4UtilsDecompressFileToBuffers(ifname, out_buffers, out_sizes, 1);
}

// ============================= Lz4UtilsInStream =============================

struct Lz4UtilsInStream {
    FILE *f_in;
    LZ4F_dctx *dctx;
    void *buf;
    size_t buf_offset;
    size_t buf_size;
    size_t lz4f_code;
};

// ========================== Lz4UtilsInStreamCreate ==========================

Lz4UtilsInStream *Lz4UtilsInStreamCreate(const char *ifname) {
    if (ifname == NULL) return NULL;
    Lz4UtilsInStream *ret =
        (Lz4UtilsInStream *)calloc(1, sizeof(Lz4UtilsInStream));
    if (!ret) return NULL;

    ret->f_in = fopen(ifname, "rb");
    if (ret->f_in == NULL) {
        free(ret);
        return NULL;
    }

    ret->buf = malloc(kInChunkSize);
    if (ret->buf == NULL) {
        fclose(ret->f_in);
        free(ret);
        return NULL;
    }

    size_t const dctx_status =
        LZ4F_createDecompressionContext(&ret->dctx, LZ4F_VERSION);
    if (Lz4fIsErrorExplained(dctx_status)) {
        free(ret->buf);
        fclose(ret->f_in);
        free(ret);
        return NULL;
    }

    ret->lz4f_code = 1;

    return ret;
}

// ============================ Lz4UtilsInStreamRun ============================

int64_t Lz4UtilsInStreamRun(Lz4UtilsInStream *stream, void *out,
                            size_t out_size) {
    if (stream == NULL || out == NULL) return -1;

    size_t initial_out_size = out_size;

    // If frame is already complete, return 0 (EOF)
    if (stream->lz4f_code == 0 && stream->buf_offset >= stream->buf_size) {
        return 0;
    }

    while (stream->lz4f_code != 0 && out_size > 0) {
        // Decompress whatever is currently available in the buffer
        while (stream->lz4f_code != 0 &&
               stream->buf_offset < stream->buf_size && out_size > 0) {
            size_t dest_buffer_size = out_size;
            size_t src_buffer_size = stream->buf_size - stream->buf_offset;
            void *src_begin =
                GenericPointerShift(stream->buf, stream->buf_offset);
            stream->lz4f_code =
                LZ4F_decompress(stream->dctx, out, &dest_buffer_size, src_begin,
                                &src_buffer_size, NULL);
            if (Lz4fIsErrorExplained(stream->lz4f_code)) return -4;

            out = GenericPointerShift(out, dest_buffer_size);
            out_size -= dest_buffer_size;
            stream->buf_offset += src_buffer_size;
        }

        if (stream->lz4f_code == 0 || out_size == 0) break;

        // Fetch more data if internal buffer is exhausted
        if (stream->buf_offset >= stream->buf_size) {
            stream->buf_offset = 0;
            stream->buf_size =
                fread(stream->buf, 1, kInChunkSize, stream->f_in);
            if (ferror(stream->f_in)) return -3;
            if (stream->buf_size == 0) {
                // Unexpected EOF while frame is incomplete
                if (stream->lz4f_code != 0) return -3;
                break;
            }
        }
    }

    return (int64_t)(initial_out_size - out_size);
}

// =========================== Lz4UtilsInStreamClose ===========================

int Lz4UtilsInStreamClose(Lz4UtilsInStream *stream) {
    if (stream == NULL) return 0;
    free(stream->buf);
    fclose(stream->f_in);
    LZ4F_errorCode_t code = LZ4F_freeDecompressionContext(stream->dctx);
    if (Lz4fIsErrorExplained(code)) {
        return -1;
    }
    free(stream);

    return 0;
}

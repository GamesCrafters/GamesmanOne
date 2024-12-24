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

#include "lz4_utils.h"

#include <lz4frame.h>  // LZ4F_*
#include <stddef.h>    // size_t, NULL
#include <stdint.h>    // int64_t
#include <stdio.h>     // FILE, fopen, fread, fclose
#include <stdlib.h>    // malloc, free

// ================================= Constants =================================

// IO buffer size.
static const int kInChunkSize = 16 << 10;

// A template for LZ4 compression preferences.
static const LZ4F_preferences_t kLz4PreferencesTemplate = {
    {LZ4F_max256KB, LZ4F_blockLinked, LZ4F_noContentChecksum, LZ4F_frame,
     0 /* unknown content size */, 0 /* no dictID */, LZ4F_noBlockChecksum},
    0,         /* compression level; 0 == default */
    0,         /* autoflush */
    0,         /* favor decompression speed */
    {0, 0, 0}, /* reserved, must be set to 0 */
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

// ========================== Lz4UtilsCompressStreams ==========================

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
        while (1) {
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

int64_t Lz4UtilsCompressStreams(const void *const *in, const size_t *in_sizes,
                                int n, int level, const char *ofname) {
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

// ========================== Lz4UtilsCompressStream ==========================

int64_t Lz4UtilsCompressStream(const void *in, size_t in_size, int level,
                               const char *ofname) {
    const void *inputs[] = {in};
    const size_t input_sizes[] = {in_size};

    return Lz4UtilsCompressStreams(inputs, input_sizes, 1, level, ofname);
}

// =========================== Lz4UtilsCompressFile ===========================

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
    while (1) {
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

int64_t Lz4UtilsCompressFile(const char *ifname, int level,
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

// ===================== Lz4UtilsDecompressFileMultistream =====================

// LZ4F_decompress has a somewhat confusing API. The 3rd
// (dstSizePtr) and the 5th (srcSizePtr) parameters are used as
// input and output parameters at the same time. The initially hold
// the CAPACITIES of the buffers and are set to the number of
// ACTUALLY CONSUMED bytes after the function call.
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

    while (lz4f_code != 0) {
        size_t read_size =
            fread(src, 1, src_capacity, f_in);  // Load more input
        const void *src_begin = src;
        const void *const src_end = (const char *)src + read_size;
        if (read_size == 0 || ferror(f_in)) return -3;

        // Continue while there is more input in buffer and the frame isn't
        // over.
        while (src_begin < src_end && lz4f_code != 0) {
            // Distribute decompressed data across streams in a round-robin
            // manner
            for (; out_index < n; ++out_index, out_offset = 0) {
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

                // Break early if input is fully consumed for this round
                if (src_begin >= src_end) break;
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

int64_t Lz4UtilsDecompressFileMultistream(const char *ifname, void **out,
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

// ========================== Lz4UtilsDecompressFile ==========================

int64_t Lz4UtilsDecompressFile(const char *ifname, void *out, size_t out_size) {
    void *out_buffers[] = {out};
    const size_t out_sizes[] = {out_size};

    return Lz4UtilsDecompressFileMultistream(ifname, out_buffers, out_sizes, 1);
}

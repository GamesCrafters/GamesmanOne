#include "lz4_utils.h"

#include <assert.h>    // assert
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
    return (void *)((uint8_t *)p + n);
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

// ========================== Lz4UtilsCompressStream ==========================

static int64_t CompressStreamInternal(const void *in, size_t in_size,
                                      FILE *f_out, LZ4F_cctx *ctx,
                                      const LZ4F_preferences_t *pref,
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
    size_t processed = 0;
    while (1) {
        const size_t read_size = SizeMin(inbuf_size, in_size - processed);
        if (read_size == 0) break;  // Nothing left in input buffer.
        const void *next_in = GenericPointerShift(in, processed);
        size_t compressed_size = LZ4F_compressUpdate(ctx, outbuf, outbuf_size,
                                                     next_in, read_size, NULL);
        if (Lz4fIsErrorExplained(compressed_size)) return -2;
        n = fwrite(outbuf, 1, compressed_size, f_out);
        if (n != compressed_size) return -3;
        processed += read_size;
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

static int64_t CompressStreamHelper(const void *in, size_t in_size, int level,
                                    FILE *f_out) {
    assert(f_out != NULL);
    int64_t ret;

    // Resource allocation.
    LZ4F_cctx *ctx;
    const LZ4F_errorCode_t ctx_creation =
        LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);  // may fail
    LZ4F_preferences_t preferences = kLz4PreferencesTemplate;
    preferences.compressionLevel = level;
    const size_t outbuf_size = LZ4F_compressBound(kInChunkSize, &preferences);
    void *const outbuf = malloc(outbuf_size);  // may fail
    if (LZ4F_isError(ctx_creation) || outbuf == NULL) {
        ret = -1;
        goto _bailout;
    }

    ret = CompressStreamInternal(in, in_size, f_out, ctx, &preferences,
                                 kInChunkSize, outbuf, outbuf_size);

_bailout:
    LZ4F_freeCompressionContext(ctx); /* supports free on NULL */
    free(outbuf);
    return ret;
}

int64_t Lz4UtilsCompressStream(const void *in, size_t in_size, int level,
                               const char *ofname) {
    if (in == NULL && in_size > 0) return -1;

    FILE *const f_out = fopen(ofname, "wb");
    if (f_out == NULL) return -3;

    const int64_t ret = CompressStreamHelper(in, in_size, level, f_out);
    fclose(f_out);

    return ret;
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

static int64_t CompressFileHelper(FILE *f_in, int level, FILE *f_out) {
    assert(f_in != NULL && f_out != NULL);
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
    if (LZ4F_isError(ctx_creation) || inbuf == NULL || outbuf == NULL) {
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

    const int64_t ret = CompressFileHelper(f_in, level, f_out);
    fclose(f_in);
    fclose(f_out);

    return ret;
}

// ========================== Lz4UtilsDecompressFile ==========================

static int64_t DecompressFileInternal(FILE *f_in, void *out, size_t out_size,
                                      LZ4F_dctx *dctx, void *src,
                                      size_t src_capacity) {
    int64_t uncompressed_size = 0;
    size_t lz4f_code = 1;
    while (lz4f_code != 0) {
        // Load more input.
        size_t read_size = fread(src, 1, src_capacity, f_in);
        const void *src_begin = src;
        const void *const src_end = (const char *)src + read_size;
        if (read_size == 0 || ferror(f_in)) return -3;

        // Continue while there is more input to read (src_begin != src_end) and
        // the frame isn't over (ret != 0).
        while (src_begin < src_end && lz4f_code != 0) {
            // LZ4F_decompress has a somewhat confusing API. The 3rd
            // (dstSizePtr) and the 5th (srcSizePtr) parameters are used as
            // input and output parameters at the same time. The initially hold
            // the CAPACITIES of the buffers and are set to the number of
            // ACTUALLY CONSUMED bytes after the function call.
            size_t to_decomp = (const char *)src_end - (const char *)src_begin;
            void *out_begin = (char *)out + uncompressed_size;
            size_t decompressed = out_size - uncompressed_size;
            lz4f_code = LZ4F_decompress(dctx, out_begin, &decompressed,
                                        src_begin, &to_decomp, NULL);
            if (Lz4fIsErrorExplained(lz4f_code)) return -4;
            uncompressed_size += decompressed;
            src_begin = GenericPointerShift(src_begin, to_decomp);
        }
        assert(src_begin <= src_end);
    }

    return uncompressed_size;
}

static int64_t DecompressFileHelper(FILE *f_in, void *out, size_t out_size) {
    assert(f_in != NULL);

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

    int64_t const result =
        DecompressFileInternal(f_in, out, out_size, dctx, src, kInChunkSize);

    free(src);
    LZ4F_freeDecompressionContext(dctx);
    return result;
}

int64_t Lz4UtilsDecompressFile(const char *ifname, void *out, size_t out_size) {
    FILE *const f_in = fopen(ifname, "rb");
    if (f_in == NULL) return -1;

    const int64_t ret = DecompressFileHelper(f_in, out, out_size);
    fclose(f_in);

    return ret;
}

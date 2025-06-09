/**
 * @file xzra.c
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

#include "libs/xzra/xzra.h"

#include <errno.h>
#include <inttypes.h>
#include <lzma.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ========================= Common Helper Functions ==========================

static const char *LzmaStreamEncoderMtRetDesc(lzma_ret ret) {
    switch (ret) {
        case LZMA_MEM_ERROR:
            return "Memory allocation failed";
        case LZMA_OPTIONS_ERROR:
            return "Specified filter chain is not supported";
        case LZMA_UNSUPPORTED_CHECK:
            return "Specified integrity check is not supported";
        default:
            break;
    }

    return "Unknown error, possibly a bug";
}

// https://github.com/tukaani-project/xz/blob/master/doc/examples/03_compress_custom.c.
static bool InitEncoder(lzma_stream *strm, uint64_t block_size, uint32_t level,
                        bool extreme, int num_threads) {
    // Set up filters.
    lzma_options_lzma opt_lzma2;
    lzma_lzma_preset(&opt_lzma2, extreme ? level | LZMA_PRESET_EXTREME : level);
    opt_lzma2.dict_size = block_size;
    lzma_filter filters[] = {
        {.id = LZMA_FILTER_LZMA2, .options = &opt_lzma2},
        {.id = LZMA_VLI_UNKNOWN, .options = NULL},
    };

    // Set up LZMA multithreading options.
    lzma_mt mt = {
        .flags = 0,  // No flags are currently supported.
        .block_size = block_size,
        .timeout = 0,
        .check = LZMA_CHECK_CRC64,
        .filters = filters,
    };

    mt.threads = num_threads == 0 ? lzma_cputhreads() : (uint32_t)num_threads;
    if (mt.threads == 0) {
        fprintf(stderr,
                "Warining: failed to get number of CPU threads, using a single "
                "thread\n");
        mt.threads = 1;
    }

    // Initialize the threaded encoder.
    lzma_ret ret = lzma_stream_encoder_mt(strm, &mt);
    if (ret == LZMA_OK) return true;

    fprintf(stderr, "Error initializing the encoder: %s (error code %u)\n",
            LzmaStreamEncoderMtRetDesc(ret), ret);
    return false;
}

static const char *LzmaCompressRetDesc(lzma_ret ret) {
    switch (ret) {
        case LZMA_OK:
            return "No error";

        case LZMA_MEM_ERROR:
            return "Memory allocation failed";

        case LZMA_DATA_ERROR:
            return "File size limits exceeded";

        default:
            return "Unknown error, possibly a bug";
    }
}

// ========================= XzraCompressionMemUsage ==========================

uint64_t XzraCompressionMemUsage(uint64_t block_size, uint32_t level,
                                 bool extreme, int num_threads) {
    // Set up filters.
    lzma_options_lzma opt_lzma2;
    lzma_lzma_preset(&opt_lzma2, extreme ? level | LZMA_PRESET_EXTREME : level);
    opt_lzma2.dict_size = block_size;
    lzma_filter filters[] = {
        {.id = LZMA_FILTER_LZMA2, .options = &opt_lzma2},
        {.id = LZMA_VLI_UNKNOWN, .options = NULL},
    };

    // Set up LZMA multithreading options.
    lzma_mt mt = {
        .flags = 0,  // No flags are currently supported.
        .block_size = block_size,
        .timeout = 0,
        .check = LZMA_CHECK_CRC64,
        .filters = filters,
    };

    mt.threads = num_threads == 0 ? lzma_cputhreads() : (uint32_t)num_threads;
    if (mt.threads == 0) {
        fprintf(stderr,
                "Warining: failed to get number of CPU threads, using a single "
                "thread\n");
        mt.threads = 1;
    }

    return lzma_stream_encoder_mt_memusage(&mt);
}

// ============================= XzraCompressFile ==============================

static bool CompressFileHelper(lzma_stream *strm, FILE *infile, FILE *outfile) {
    lzma_action action = LZMA_RUN;
    uint8_t inbuf[BUFSIZ];
    uint8_t outbuf[BUFSIZ];
    strm->next_in = NULL;
    strm->avail_in = 0;
    strm->next_out = outbuf;
    strm->avail_out = sizeof(outbuf);
    while (true) {
        if (strm->avail_in == 0 && !feof(infile)) {
            strm->next_in = inbuf;
            strm->avail_in = fread(inbuf, 1, sizeof(inbuf), infile);
            if (ferror(infile)) {
                strm->next_in = strm->next_out = NULL;
                strerror_r(errno, (char *)outbuf, sizeof(outbuf));
                fprintf(stderr, "XzraCompressFile: Read error: %s\n", outbuf);
                return false;
            }
            if (feof(infile)) action = LZMA_FINISH;
        }
        lzma_ret ret = lzma_code(strm, action);
        if (strm->avail_out == 0 || ret == LZMA_STREAM_END) {
            size_t write_size = sizeof(outbuf) - strm->avail_out;
            if (fwrite(outbuf, 1, write_size, outfile) != write_size) {
                strm->next_in = strm->next_out = NULL;
                strerror_r(errno, (char *)outbuf, sizeof(outbuf));
                fprintf(stderr, "XzraCompressFile: Write error: %s\n", outbuf);
                return false;
            }
            strm->next_out = outbuf;
            strm->avail_out = sizeof(outbuf);
        }
        if (ret != LZMA_OK) {
            strm->next_in = strm->next_out = NULL;
            if (ret == LZMA_STREAM_END) return true;

            const char *msg = LzmaCompressRetDesc(ret);
            fprintf(stderr,
                    "XzraCompressFile: Encoder error: %s (error code %u)\n",
                    msg, ret);
            return false;
        }
    }
}

int64_t XzraCompressFile(const char *ofname, uint64_t block_size,
                         uint32_t level, bool extreme, int num_threads,
                         const char *ifname) {
    FILE *infile = fopen(ifname, "rb");
    if (infile == NULL) {
        fprintf(stderr, "XzraCompressFile: failed to open input file\n");
        return -1;
    }
    FILE *outfile = fopen(ofname, "wb");
    if (outfile == NULL) {
        fprintf(stderr, "XzraCompressFile: failed to open output file\n");
        fclose(infile);
        return -2;
    }

    lzma_stream strm = LZMA_STREAM_INIT;
    bool success =
        InitEncoder(&strm, block_size, level, extreme, num_threads) &&
        CompressFileHelper(&strm, infile, outfile);
    int64_t ret = success ? (int64_t)strm.total_out : -3;
    lzma_end(&strm);
    if (fclose(outfile)) {
        char buf[BUFSIZ];
        strerror_r(errno, buf, sizeof(buf));
        fprintf(stderr, "XzraCompressFile: write error: %s\n", buf);
        ret = -3;
    }
    fclose(infile);

    return ret;
}

// ============================= XzraCompressMem ==============================

static bool CompressMemHelper(lzma_stream *strm, const uint8_t *in,
                              size_t in_size, FILE *outfile) {
    uint8_t outbuf[BUFSIZ];
    strm->next_in = in;
    strm->avail_in = in_size;
    strm->next_out = outbuf;
    strm->avail_out = sizeof(outbuf);
    while (true) {
        lzma_ret ret = lzma_code(strm, LZMA_FINISH);
        if (strm->avail_out == 0 || ret == LZMA_STREAM_END) {
            size_t write_size = sizeof(outbuf) - strm->avail_out;
            if (fwrite(outbuf, 1, write_size, outfile) != write_size) {
                strm->next_out = NULL;
                strerror_r(errno, (char *)outbuf, sizeof(outbuf));
                fprintf(stderr, "XzraCompressMem: Write error: %s\n", outbuf);
                return false;
            }
            strm->next_out = outbuf;
            strm->avail_out = sizeof(outbuf);
        }
        if (ret != LZMA_OK) {
            strm->next_out = NULL;
            if (ret == LZMA_STREAM_END) return true;
            const char *msg = LzmaCompressRetDesc(ret);
            fprintf(stderr,
                    "XzraCompressMem: Encoder error: %s (error code %u)\n", msg,
                    ret);
            return false;
        }
    }
}

int64_t XzraCompressMem(const char *ofname, uint64_t block_size, uint32_t level,
                        bool extreme, int num_threads, const uint8_t *in,
                        size_t in_size) {
    FILE *outfile = fopen(ofname, "wb");
    if (outfile == NULL) {
        fprintf(stderr, "XzraCompressMem: failed to open output file %s\n",
                ofname);
        return -2;
    }

    lzma_stream strm = LZMA_STREAM_INIT;
    bool success =
        InitEncoder(&strm, block_size, level, extreme, num_threads) &&
        CompressMemHelper(&strm, in, in_size, outfile);
    int64_t ret = success ? (int64_t)strm.total_out : -3;
    lzma_end(&strm);
    if (fclose(outfile)) {
        char buf[BUFSIZ];
        strerror_r(errno, buf, sizeof(buf));
        fprintf(stderr, "XzraCompressMem: write error: %s\n", buf);
        ret = -3;
    }

    return ret;
}

// ============================== XzraOutStream ===============================

struct XzraOutStream {
    uint8_t outbuf[BUFSIZ]; /**< Buffer for output. */
    FILE *outfile;          /**< Output file. */
    lzma_stream strm;       /**< LZMA internal stream. */
};

// =========================== XzraOutStreamCreate ============================

XzraOutStream *XzraOutStreamCreate(const char *ofname, uint64_t block_size,
                                   uint32_t level, bool extreme,
                                   int num_threads) {
    XzraOutStream *ret = (XzraOutStream *)malloc(sizeof(XzraOutStream));
    if (ret == NULL) return NULL;

    ret->outfile = fopen(ofname, "wb");
    if (ret->outfile == NULL) {
        fprintf(stderr, "XzraOutStreamCreate: failed to open output file %s\n",
                ofname);
        free(ret);
        return NULL;
    }

    ret->strm = (lzma_stream)LZMA_STREAM_INIT;
    if (!InitEncoder(&ret->strm, block_size, level, extreme, num_threads)) {
        free(ret);
        return NULL;
    }

    ret->strm.next_out = ret->outbuf;
    ret->strm.avail_out = sizeof(ret->outbuf);

    return ret;
}

// ============================= XzraOutStreamRun ==============================

int64_t XzraOutStreamRun(XzraOutStream *stream, const uint8_t *in,
                         size_t in_size) {
    stream->strm.next_in = in;
    stream->strm.avail_in = in_size;
    while (stream->strm.avail_in) {
        lzma_ret ret = lzma_code(&stream->strm, LZMA_RUN);
        if (stream->strm.avail_out == 0) {
            size_t write_size = sizeof(stream->outbuf);
            if (fwrite(stream->outbuf, 1, write_size, stream->outfile) !=
                write_size) {
                strerror_r(errno, (char *)stream->outbuf,
                           sizeof(stream->outbuf));
                fprintf(stderr, "XzraOutStreamRun: write error: %s\n",
                        stream->outbuf);
                return -3;
            }
            stream->strm.next_out = stream->outbuf;
            stream->strm.avail_out = sizeof(stream->outbuf);
        }
        if (ret != LZMA_OK) {
            const char *msg = LzmaCompressRetDesc(ret);
            fprintf(stderr,
                    "XzraOutStreamClose: encoder error: %s (error code %u)\n",
                    msg, ret);
            return -3;
        }
    }

    return (int64_t)stream->strm.total_out;
}

// ============================ XzraOutStreamClose =============================

int64_t XzraOutStreamClose(XzraOutStream *stream) {
    if (stream == NULL) return 0;

    stream->strm.next_in = NULL;
    stream->strm.avail_in = 0;
    while (true) {
        lzma_ret l_ret = lzma_code(&stream->strm, LZMA_FINISH);
        if (stream->strm.avail_out == 0 || l_ret == LZMA_STREAM_END) {
            size_t write_size = sizeof(stream->outbuf) - stream->strm.avail_out;
            if (fwrite(stream->outbuf, 1, write_size, stream->outfile) !=
                write_size) {
                strerror_r(errno, (char *)stream->outbuf,
                           sizeof(stream->outbuf));
                fprintf(stderr, "XzraOutStreamClose: write error: %s\n",
                        stream->outbuf);
                free(stream);
                return -3;
            }
            stream->strm.next_out = stream->outbuf;
            stream->strm.avail_out = sizeof(stream->outbuf);
        }
        if (l_ret != LZMA_OK) {
            if (l_ret == LZMA_STREAM_END) break;

            const char *msg = LzmaCompressRetDesc(l_ret);
            fprintf(stderr,
                    "XzraOutStreamClose: encoder error: %s (error code %u)\n",
                    msg, l_ret);
            free(stream);
            return -3;
        }
    }
    int64_t ret = (int64_t)stream->strm.total_out;
    lzma_end(&stream->strm);
    if (fclose(stream->outfile)) {
        strerror_r(errno, (char *)stream->outbuf, sizeof(stream->outbuf));
        fprintf(stderr, "XzraOutStreamClose: write error: %s\n",
                stream->outbuf);
        ret = -3;
    }
    free(stream);

    return ret;
}

// ======================== XzraDecompressionMemUsage =========================

uint64_t XzraDecompressionMemUsage(uint64_t block_size, uint32_t level,
                                   bool extreme, int num_threads) {
    // Set up filters.
    lzma_options_lzma opt_lzma2;
    lzma_lzma_preset(&opt_lzma2, extreme ? level | LZMA_PRESET_EXTREME : level);
    opt_lzma2.dict_size = block_size;
    lzma_filter filters[] = {
        {.id = LZMA_FILTER_LZMA2, .options = &opt_lzma2},
        {.id = LZMA_VLI_UNKNOWN, .options = NULL},
    };

    return lzma_raw_decoder_memusage(filters) * num_threads;
}

// ============================ XzraDecompressFile =============================

static const char *LzmaStreamDecoderMtRetDesc(lzma_ret ret) {
    switch (ret) {
        case LZMA_MEM_ERROR:
            return "Memory allocation failed";
        case LZMA_OPTIONS_ERROR:
            return "Unsupported decompressor flags";
        default:
            break;
    }
    return "Unknown error, possibly a bug";
}

static bool InitDecoder(lzma_stream *strm, uint32_t num_threads,
                        uint64_t memlimit) {
    lzma_mt mt = {
        .flags = LZMA_CONCATENATED | LZMA_FAIL_FAST,
        .timeout = 0,
        .memlimit_threading = UINT64_MAX,  // No memory limit on threading.
        .memlimit_stop = memlimit,         // Fail if exceeding memory limit.
    };
    mt.threads = num_threads ? num_threads : lzma_cputhreads();
    if (mt.threads == 0) mt.threads = 1;
    lzma_ret ret = lzma_stream_decoder_mt(strm, &mt);
    if (ret != LZMA_OK) {
        fprintf(stderr, "Error initializing the decoder: %s (error code %u)\n",
                LzmaStreamDecoderMtRetDesc(ret), ret);
        return false;
    }

    return true;
}

static const char *DecompressHelperLzmaCodeRetDesc(lzma_ret ret) {
    switch (ret) {
        case LZMA_MEM_ERROR:
            return "Memory allocation failed";
        case LZMA_FORMAT_ERROR:
            return "The input is not in the .xz format";
        case LZMA_OPTIONS_ERROR:
            return "Unsupported compression options";
        case LZMA_DATA_ERROR:
            return "Compressed file is corrupt";
        case LZMA_BUF_ERROR:
            return "Compressed file is truncated or otherwise corrupt";
        default:
            break;
    }
    return "Unknown error, possibly a bug";
}

static bool DecompressFileHelper(lzma_stream *strm, FILE *infile, uint8_t *dest,
                                 size_t size) {
    lzma_action action = LZMA_RUN;
    uint8_t inbuf[BUFSIZ];
    strm->next_in = NULL;
    strm->avail_in = 0;
    strm->next_out = dest;
    strm->avail_out = size;
    while (true) {
        if (strm->avail_in == 0 && !feof(infile)) {
            strm->next_in = inbuf;
            strm->avail_in = fread(inbuf, 1, sizeof(inbuf), infile);
            if (ferror(infile)) {
                strm->next_in = NULL;
                strerror_r(errno, (char *)inbuf, sizeof(inbuf));
                fprintf(stderr, "Read error: %s\n", inbuf);
                return false;
            }
            if (feof(infile)) action = LZMA_FINISH;
        }
        lzma_ret ret = lzma_code(strm, action);
        if (ret == LZMA_STREAM_END || strm->avail_out == 0) {
            strm->next_in = NULL;
            return true;
        }
        if (ret != LZMA_OK) {
            strm->next_in = NULL;
            fprintf(stderr, "Decoder error: %s (error code %u)\n",
                    DecompressHelperLzmaCodeRetDesc(ret), ret);
            return false;
        }
    }
}

int64_t XzraDecompressFile(uint8_t *dest, size_t size, int num_threads,
                           uint64_t memlimit, const char *filename) {
    lzma_stream strm = LZMA_STREAM_INIT;
    if (!InitDecoder(&strm, num_threads, memlimit)) return -1;
    FILE *infile = fopen(filename, "rb");
    if (infile == NULL) {
        char buf[BUFSIZ];
        strerror_r(errno, buf, sizeof(buf));
        fprintf(stderr, "XzraDecompressFile: error opening %s: %s\n", filename,
                buf);
        return -2;
    }
    bool success = DecompressFileHelper(&strm, infile, dest, size);
    bool fclose_success = fclose(infile) == 0;
    int64_t total_out = (int64_t)strm.total_out;
    lzma_end(&strm);
    if (!success) return -3;
    if (!fclose_success) return -4;

    return total_out;
}

// ================================= XzraFile ==================================

/** @brief Container for an XZ block. */
typedef struct XzraBlock {
    /**
     * @brief Uncompressed block content. The XzraBlock is invalid if this field
     * is \c NULL.
     */
    uint8_t *uncompressed_data;

    /**
     * @brief Capacity of the \p uncompressed_data buffer.
     */
    size_t capacity;
} XzraBlock;

/** @brief Read-only XZ file with random access ability. */
struct XzraFile {
    FILE *file;        /**< Kept open until the XzraFile is closed. */
    lzma_index *index; /**< XZ file index, valid while the XzraFile is open. */
    lzma_index_iter iter; /**< XZ block iterator. */
    XzraBlock block; /**< Cached XZ block. Uninitialized until first read. */
    size_t pos;      /**< Uncompressed file position indicator. */
    bool eof;        /**< EOF flag. */
};

// =============================== XzraFileOpen ================================

static const char *LzmaStreamFooterDecodeRetDesc(lzma_ret ret) {
    switch (ret) {
        case LZMA_FORMAT_ERROR:
            return "Magic bytes don't match, thus the given buffer cannot be "
                   "Stream Footer";
        case LZMA_DATA_ERROR:
            return "CRC32 doesn't match, thus the Stream Footer is corrupt";
        case LZMA_OPTIONS_ERROR:
            return "Unsupported options are present in Stream Footer";
        default:
            break;
    }
    return "Unknown error, possibly a bug";
}

static lzma_vli GetBackwardSize(FILE *f) {
    // The footer is always the same length as the header, which is 12 bytes
    // long according to the xz file format:
    // https://github.com/tukaani-project/xz/blob/master/doc/xz-file-format.txt.
    uint8_t buf[LZMA_STREAM_HEADER_SIZE];
    fseek(f, -LZMA_STREAM_HEADER_SIZE, SEEK_END);  // Seek to the footer.
    size_t count = fread(buf, 1, LZMA_STREAM_HEADER_SIZE,
                         f);  // Read the footer into the buffer.
    if (count != LZMA_STREAM_HEADER_SIZE) {
        fprintf(stderr, "failed to read footer\n");
        return LZMA_VLI_UNKNOWN;
    }

    // Parse footer
    lzma_stream_flags footer;
    lzma_ret ret = lzma_stream_footer_decode(&footer, buf);
    if (ret != LZMA_OK) {
        fprintf(stderr,
                "lzma_stream_footer_decode error failed with code %d: %s\n",
                ret, LzmaStreamFooterDecodeRetDesc(ret));
        return LZMA_VLI_UNKNOWN;
    }

    return footer.backward_size;
}

static const char *LzmaIndexBufferDecodeRetDesc(lzma_ret ret) {
    switch (ret) {
        case LZMA_MEM_ERROR:
            return "LZMA_MEM_ERROR (no description available)";
        case LZMA_MEMLIMIT_ERROR:
            return "Memory usage limit was reached, possibly a bug because no "
                   "limit was set";
        case LZMA_DATA_ERROR:
            return "LZMA_DATA_ERROR (no description available)";
        case LZMA_PROG_ERROR:
            return "LZMA_PROG_ERROR (no description available)";
        default:
            break;
    }
    return "Unknown error, possibly a bug";
}

static int XzraGetIndex(lzma_index **index, FILE *f) {
    lzma_vli backward_size = GetBackwardSize(f);
    if (backward_size == LZMA_VLI_UNKNOWN) return 1;

    fseek(f, -LZMA_STREAM_HEADER_SIZE - (int64_t)backward_size, SEEK_END);
    uint8_t *buf = (uint8_t *)malloc(backward_size * sizeof(uint8_t));
    if (buf == NULL) return 2;

    size_t count = fread(buf, 1, backward_size, f);
    if (count != backward_size) {
        free(buf);
        return 3;
    }

    uint64_t memlimit = UINT64_MAX;  // Unlimited.
    size_t in_pos = 0;
    lzma_ret ret = lzma_index_buffer_decode(index, &memlimit, NULL, buf,
                                            &in_pos, backward_size);
    free(buf);
    if (ret != LZMA_OK) {
        fprintf(stderr, "lzma_index_buffer_decode failed with code %d: %s\n",
                ret, LzmaIndexBufferDecodeRetDesc(ret));
        return 4;
    }

    return 0;  // Success.
}

static const char *XzraGetIndexErrorDesc(int error) {
    switch (error) {
        case 1:
            return "failing to get backward size";
        case 2:
            return "failing to malloc buffer for index";
        case 3:
            return "failing to read index from file";
        case 4:
            return "index decompression failure";
        default:
            break;
    }
    return "unknown error";
}

XzraFile *XzraFileOpen(const char *filename) {
    // Allocate memory for the return value.
    XzraFile *ret = (XzraFile *)calloc(1, sizeof(XzraFile));
    if (ret == NULL) {
        fprintf(stderr, "XzraFileOpen: malloc failed\n");
        return NULL;
    }

    // Open XZ file.
    ret->file = fopen(filename, "rb");
    if (ret->file == NULL) {
        fprintf(stderr, "XzraFileOpen: failed to open %s\n", filename);
        free(ret);
        return NULL;
    }

    // Load XZ index.
    int error = XzraGetIndex(&ret->index, ret->file);
    if (error != 0) {
        fprintf(stderr, "XzraFileOpen: failed to load index of %s due to %s\n",
                filename, XzraGetIndexErrorDesc(error));
        fclose(ret->file);
        free(ret);
        return NULL;
    }

    return ret;
}

// =============================== XzraFileClose ===============================

int XzraFileClose(XzraFile *file) {
    if (file == NULL) return 0;

    free(file->block.uncompressed_data);
    lzma_index_end(file->index, NULL);
    int ret = fclose(file->file);
    if (ret != 0) fprintf(stderr, "XzraFileClose: failed to close file\n");
    free(file);

    return ret;
}

// =============================== XzraFileSeek ================================

int XzraFileSeek(XzraFile *file, int64_t offset, int origin) {
    switch (origin) {
        case XZRA_SEEK_SET:
            file->pos = offset;
            break;
        case XZRA_SEEK_CUR:
            file->pos += offset;
            break;
        default:
            fprintf(stderr, "XzraFileSeek: unsupported seek origin\n");
            return -1;
    }

    file->eof = false;
    return 0;
}

// =============================== XzraFileRead ================================

static int XzraRetrieveBlock(lzma_index_iter *iter, const lzma_index *index,
                             size_t uncompressed_target_offset) {
    lzma_index_iter_init(iter, index);  // Initialize index iterator
    bool failed = lzma_index_iter_locate(iter, uncompressed_target_offset);

    return failed ? 1 : 0;
}

static const char *LzmaBlockHeaderDecodeRetDesc(lzma_ret ret) {
    switch (ret) {
        case LZMA_OPTIONS_ERROR:
            return "The Block Header specifies some unsupported options such "
                   "as unsupported filters, likely a bug";
        case LZMA_DATA_ERROR:
            return "Block Header is corrupt, for example, the CRC32 doesn't "
                   "match";
        case LZMA_PROG_ERROR:
            return "Invalid arguments, likely a bug";
        default:
            break;
    }
    return "Unknown error, possibly a bug";
}

static bool DecodeBlockHeader(lzma_block *block, uint8_t *block_buf) {
    block->header_size = lzma_block_header_size_decode(block_buf[0]);
    lzma_ret ret = lzma_block_header_decode(block, NULL, block_buf);
    if (ret != LZMA_OK) {
        const char *msg = LzmaBlockHeaderDecodeRetDesc(ret);
        fprintf(stderr, "lzma_block_header_decode failed with code %d: %s\n",
                ret, msg);
    }

    return ret == LZMA_OK;
}

static const char *LzmaBlockBufferDecodeRetDesc(lzma_ret ret) {
    switch (ret) {
        case LZMA_OPTIONS_ERROR:
            return "LZMA_OPTIONS_ERROR (no description available)";
        case LZMA_DATA_ERROR:
            return "LZMA_DATA_ERROR (no description available)";
        case LZMA_MEM_ERROR:
            return "LZMA_MEM_ERROR (no description available)";
        case LZMA_BUF_ERROR:
            return "Output buffer was too small, likely a bug";
        case LZMA_PROG_ERROR:
            return "LZMA_PROG_ERROR (no description available)";
        default:
            break;
    }
    return "Unknown error, possibly a bug";
}

static bool DecodeBlock(uint8_t *out, lzma_block *block,
                        int64_t compressed_size, const uint8_t *block_buf) {
    size_t in_pos = block->header_size;
    size_t out_pos = 0;
    // Note that we cannot use block->compressed_size here as the size of the
    // input buffer because it does not include padding size.
    lzma_ret ret = lzma_block_buffer_decode(block, NULL, block_buf, &in_pos,
                                            compressed_size, out, &out_pos,
                                            block->uncompressed_size);
    if (ret != LZMA_OK) {
        fprintf(stderr,
                "lzma_block_buffer_decode failed with error code: %d: %s\n",
                ret, LzmaBlockBufferDecodeRetDesc(ret));
        return false;
    }

    return true;
}

static int XzraDecodeBlock(uint8_t *out, const lzma_index_iter *iter, FILE *f) {
    // Seek to block.
    fseek(f, (int64_t)iter->block.compressed_file_offset, SEEK_SET);

    // Allocate space for compressed block.
    uint8_t *block_buf = (uint8_t *)malloc(iter->block.total_size);
    if (block_buf == NULL) return 2;

    // Read compressed block into buffer.
    size_t count = fread(block_buf, 1, iter->block.total_size, f);
    if (count != iter->block.total_size) {
        free(block_buf);
        return 3;
    }

    // Decode block header
    lzma_filter filters[LZMA_FILTERS_MAX + 1];  // Preallocate space.
    lzma_block b = {
        .version = 0,  // The API is unclear about this field, using 0 for now.
        .filters = filters,
    };
    if (!DecodeBlockHeader(&b, block_buf)) {
        free(block_buf);
        return 4;
    }

    bool success =
        DecodeBlock(out, &b, (int64_t)iter->block.total_size, block_buf);
    free(block_buf);

    return success ? 0 : 5;
}

static bool XzraFileBlockBufferHit(const XzraFile *file) {
    if (file->iter.block.number_in_file == 0) return false;  // Uninitialized.
    if (file->pos < file->iter.block.uncompressed_file_offset) return false;
    if (file->pos >= file->iter.block.uncompressed_file_offset +
                         file->iter.block.uncompressed_size) {
        return false;
    }
    return true;
}

// Loads the block that contains the byte at index file->pos, deallocating the
// old block buffer if exists.
static int XzraFileFillBlockBuffer(XzraFile *file, bool next) {
    if (next) {
        // lzma_index_iter_next returns true on failure or EOF.
        if (lzma_index_iter_next(&file->iter, LZMA_INDEX_ITER_BLOCK)) {
            return 1;
        }
    } else {
        int error = XzraRetrieveBlock(&file->iter, file->index, file->pos);
        // error == 1 is the only possibility for now.
        if (error != 0) return 1;
    }

    // Reallocate block data buffer if necessary
    size_t new_block_size = file->iter.block.uncompressed_size;
    if (file->block.capacity < new_block_size) {
        uint8_t *tmp =
            (uint8_t *)realloc(file->block.uncompressed_data, new_block_size);
        if (tmp == NULL) return 2;
        file->block.uncompressed_data = tmp;
        file->block.capacity = new_block_size;
    }

    return XzraDecodeBlock(file->block.uncompressed_data, &file->iter,
                           file->file);
}

static size_t MinSize(size_t a, size_t b) { return a < b ? a : b; }

static const char *XzraFileFillBlockBufferErrorDesc(int error) {
    switch (error) {
        case 1:
            return "reaching the end of file";
        case 2:
            return "malloc failure";
        case 3:
            return "failure to read compressed block from file";
        case 4:
            return "failure to decode block header";
        case 5:
            return "failure to decode block";
        default:
            break;
    }
    return "unknown error";
}

// Assumes file->block has been loaded and the uncompressed file position
// indicator is inside uncompressed block window.
static size_t XzraFileRemainingBlockSize(const XzraFile *file) {
    return file->iter.block.uncompressed_file_offset +
           file->iter.block.uncompressed_size - file->pos;
}

static size_t XzraFilePosOffsetInBlock(const XzraFile *file) {
    return file->pos - file->iter.block.uncompressed_file_offset;
}

size_t XzraFileRead(void *dest, size_t size, XzraFile *file) {
    size_t total_read = 0;
    if (file->eof) return total_read;
    if (size == 0) return total_read;
    if (!XzraFileBlockBufferHit(file)) {
        int error = XzraFileFillBlockBuffer(file, false);
        if (error != 0) {
            if (error == 1) {  // EOF.
                file->eof = true;
            } else {
                fprintf(stderr,
                        "XzraFileRead: failed to load block due to %s\n",
                        XzraFileFillBlockBufferErrorDesc(error));
            }
            return total_read;
        }
    }

    size_t next_read = size;
    while (next_read > 0) {
        size_t read_size = MinSize(next_read, XzraFileRemainingBlockSize(file));
        size_t offset = XzraFilePosOffsetInBlock(file);
        void *next_dest = (void *)((uint8_t *)dest + total_read);
        memcpy(next_dest, &file->block.uncompressed_data[offset], read_size);
        file->pos += read_size;
        next_read -= read_size;
        total_read += read_size;
        if (next_read > 0) {
            int error = XzraFileFillBlockBuffer(file, true);
            if (error != 0) {
                if (error == 1) {  // EOF.
                    file->eof = true;
                } else {
                    fprintf(stderr,
                            "XzraFileRead: failed to load block due to %s\n",
                            XzraFileFillBlockBufferErrorDesc(error));
                }
                return total_read;
            }
        }
    }

    return total_read;
}

// ================================ XzraFileEOF ================================

bool XzraFileEOF(const XzraFile *file) { return file->eof; }

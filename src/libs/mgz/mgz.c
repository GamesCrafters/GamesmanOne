#include "libs/mgz/mgz.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "libs/mgz/gz64.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#define PRAGMA(X) _Pragma(#X)
#define PRAGMA_OMP_PARALLEL_FOR PRAGMA(omp parallel for)

// Otherwise, the following macros do nothing.
#else
#define PRAGMA
#define PRAGMA_OMP_PARALLEL_FOR
#endif

const uInt kChunkSize = 1 << 14;  // 16 KiB
const uInt kIllegalChunkSize = kChunkSize + 1;
const int kDefaultOutCapacity = kChunkSize << 1;
const int kMgzMinBlockSize = kChunkSize;
const int64_t kDefaultBlockSize = 1 << 20;
const int64_t kIllegalBlockSize = -1;

static void *VoidpShift(const void *p, int64_t offset) {
    return (void *)((uint8_t *)p + offset);
}

static uInt LoadInput(uint8_t *inBuf, const void *in, int64_t offset,
                      int64_t *remSize) {
    uInt loadSize = 0;
    if (*remSize == 0) return 0;
    if (*remSize < kChunkSize) {
        loadSize = (uInt)*remSize;
    } else {
        loadSize = kChunkSize;
    }
    *remSize -= loadSize;
    memcpy(inBuf, VoidpShift(in, offset), loadSize);

    return loadSize;
}

static uInt CopyOutput(void **out, int64_t out_offset, int64_t *out_capacity,
                       uint8_t *out_buf, uInt have) {
    if (out_offset + have > *out_capacity) {
        /* Not enough space, reallocate output array. */
        do {
            (*out_capacity) *= 2;
        } while (out_offset + have > *out_capacity);

        void *new_out = realloc(*out, *out_capacity);
        if (!new_out) return kIllegalChunkSize;

        *out = new_out;
    }
    memcpy(VoidpShift(*out, out_offset), out_buf, have);
    return have;
}

int64_t MgzDeflate(void **out, const void *in, int64_t in_size, int level) {
    if (in_size == 0) {
        *out = NULL;
        return 0;
    }

    int z_ret = Z_OK, flush = Z_NO_FLUSH;
    int64_t in_offset = 0, out_offset = 0;
    int64_t out_capacity = kDefaultOutCapacity;
    z_stream strm;
    uint8_t *in_buf = (uint8_t *)malloc(kChunkSize);
    uint8_t *out_buf = (uint8_t *)malloc(kChunkSize);
    *out = malloc(out_capacity);
    if (!in_buf || !out_buf || !(*out)) {
        fprintf(stderr, "MgzDeflate: malloc failed.\n");
        free(*out);
        *out = NULL;
        goto _bailout;
    }

    /* Allocate deflate state. */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    z_ret = deflateInit2(&strm, level, Z_DEFLATED, 15 + 16, 8,
                         Z_DEFAULT_STRATEGY);  // +16 for gzip header.
    if (z_ret != Z_OK) {
        free(*out);
        *out = NULL;
        goto _bailout;
    }

    /* Compress until the end of IN. */
    do {
        strm.avail_in = LoadInput(in_buf, in, in_offset, &in_size);
        in_offset += strm.avail_in;
        if (strm.avail_in < kChunkSize) flush = Z_FINISH;
        strm.next_in = in_buf;

        /* Run deflate() on input until output buffer not full; finish
         * compression if all of source has been read in. */
        do {
            strm.avail_out = kChunkSize;
            strm.next_out = out_buf;
            z_ret = deflate(&strm, flush);
            if (z_ret == Z_STREAM_ERROR) {
                fprintf(stderr,
                        "MgzDeflate: (FATAL) deflate returned "
                        "Z_STREAM_ERROR.\n");
                exit(1);
            }
            uInt have = kChunkSize - strm.avail_out;
            uInt copied =
                CopyOutput(out, out_offset, &out_capacity, out_buf, have);
            if (copied == kIllegalChunkSize) {
                fprintf(stderr, "MgzDeflate: output realloc failed.\n");
                (void)deflateEnd(&strm);
                free(*out);
                *out = NULL;
                out_offset = 0;
                goto _bailout;
            }
            assert(copied == have);
            out_offset += copied;
        } while (strm.avail_out == 0);
    } while (flush != Z_FINISH);

    /* Clean up and return. */
    (void)deflateEnd(&strm);

_bailout:
    free(in_buf);
    free(out_buf);
    return out_offset;
}

static int64_t GetCorrectBlockSize(int64_t block_size) {
    if (block_size == 0) return kDefaultBlockSize;
    if (block_size < kMgzMinBlockSize) {
        printf("GetCorrectBlockSize: resetting block size %" PRId64
               " to the minimum required block size %d",
               block_size, kMgzMinBlockSize);
        return kMgzMinBlockSize;
    }
    return block_size;
}

static int64_t ConvertOutBlockSizesToLookup(int64_t *outBlockSizes,
                                            int64_t num_blocks) {
    int64_t t1 = 0, t2 = 0;
    for (int64_t i = 0; i < num_blocks; ++i) {
        t2 = t1 + outBlockSizes[i];
        outBlockSizes[i] = t1;
        t1 = t2;
    }
    outBlockSizes[num_blocks] = t1;
    return t1;
}

mgz_res_t MgzParallelDeflate(const void *in, int64_t in_size, int level,
                             int64_t block_size, bool lookup) {
    mgz_res_t ret = {0};
    block_size = GetCorrectBlockSize(block_size);
    int64_t num_blocks =
        (in_size + block_size - 1) / block_size;  // Round up division.
    if (num_blocks == 0) return ret;              // INSIZE is 0.
    void *out = NULL;

    /* Allocate space for the output of each block. */
    void **out_blocks = (void **)calloc(num_blocks, sizeof(void *));

    /* Shared space for outBlockSizes and lookup. outBlockSizes
       is later converted to lookup in-place. */
    int64_t *space = (int64_t *)malloc((num_blocks + 1) * sizeof(int64_t));
    if (!out_blocks || !space) goto _bailout;

    /* Compress each block. */
    bool oom = false;

    PRAGMA_OMP_PARALLEL_FOR
    for (int64_t i = 0; i < num_blocks; ++i) {
        int64_t thisBlockSize =
            (i == num_blocks - 1) ? in_size - i * block_size : block_size;
        space[i] = MgzDeflate(&out_blocks[i], VoidpShift(in, i * block_size),
                              thisBlockSize, level);
        if (space[i] == 0) oom = true;
    }
    if (oom) goto _bailout;

    /* Concatenate blocks to form the final output. */
    int64_t out_size = ConvertOutBlockSizesToLookup(space, num_blocks);
    out = malloc(out_size);
    if (!out) goto _bailout;

    PRAGMA_OMP_PARALLEL_FOR
    for (int64_t i = 0; i < num_blocks; ++i) {
        memcpy(VoidpShift(out, space[i]), out_blocks[i],
               space[i + 1] - space[i]);
        free(out_blocks[i]);
        out_blocks[i] = NULL;
    }

    /* Reach here only if compression was successful.
     * Setup return value. */
    ret.out = out;
    out = NULL;  // Prevent freeing.
    ret.size = out_size;
    if (lookup) {
        ret.lookup = space;
        space = NULL;  // Prevent freeing.
    }                  // else ret.lookup is already set to NULL.
    ret.num_blocks = num_blocks;

_bailout:
    free(out);
    if (out_blocks) {
        for (int64_t i = 0; i < num_blocks; ++i) {
            free(out_blocks[i]);
        }
    }
    free(out_blocks);
    free(space);
    return ret;
}

int64_t MgzParallelCreate(const void *in, int64_t size, int level,
                          int64_t block_size, FILE *outfile, FILE *lookup) {
    block_size = GetCorrectBlockSize(block_size);
    mgz_res_t res = MgzParallelDeflate(in, size, level, block_size, lookup);
    if (!res.out) return 0;
    if (fwrite(res.out, 1, res.size, outfile) != (size_t)res.size) {
        fprintf(stderr,
                "MgzParallelCreate: (FATAL) failed to write to "
                "outfile.\n");
        exit(1);
    }
    if (lookup) {
        /* Write block size. */
        if (fwrite(&block_size, sizeof(int64_t), 1, lookup) != 1) {
            fprintf(stderr,
                    "MgzParallelCreate: (FATAL) failed to write to "
                    "lookup.\n");
            exit(1);
        }

        /* Write lookup table. */
        if (fwrite(res.lookup, sizeof(int64_t), res.num_blocks, lookup) !=
            (size_t)res.num_blocks) {
            fprintf(stderr,
                    "MgzParallelCreate: (FATAL) failed to write to "
                    "lookup.\n");
            exit(1);
        }
    }
    free(res.out);
    free(res.lookup);
    return res.size;
}

int64_t MgzRead(void *buf, int64_t size, int64_t offset, int fd, FILE *lookup) {
    if (!buf || !size || !lookup) return 0;

    /* Read block size from lookup file. */
    int64_t block_size;
    if (fseek(lookup, 0, SEEK_SET) < 0) {
        fprintf(stderr,
                "MgzRead: failed to seek to the beginning of lookup "
                "file.\n");
        return 0;
    }
    if (fread(&block_size, sizeof(int64_t), 1, lookup) != 1) {
        fprintf(stderr, "MgzRead: failed to read block size from lookup.\n");
        return 0;
    }

    /* Seek to the correct location. */
    int64_t block = offset / block_size;
    off_t into = offset % block_size;
    if (fseek(lookup, block * sizeof(int64_t), SEEK_CUR) < 0) {
        fprintf(stderr,
                "MgzRead: failed to seek to the given block in lookup "
                "file.\n");
        return 0;
    }
    int64_t gz_off;
    if (fread(&gz_off, sizeof(int64_t), 1, lookup) != 1) {
        fprintf(stderr, "MgzRead: failed to read gz offset from lookup.\n");
        return 0;
    }

    if (lseek(fd, gz_off, SEEK_SET) != (off_t)gz_off) {
        fprintf(stderr, "MgzRead: lseek failed.\n");
        return 0;
    }
    int gzfd = dup(fd);
    if (gzfd == -1) {
        fprintf(stderr, "MgzRead: failed to dup() fd.\n");
        return 0;
    }
    gzFile archive = gzdopen(gzfd, "rb");
    if (!archive) {
        fprintf(stderr, "MgzRead: failed to gzdopen() the new fd.\n");
        return 0;
    }
    if (gzseek(archive, into, SEEK_CUR) != into) {
        fprintf(stderr,
                "MgzRead: gzseek not returning an offset equal to "
                "into.\n");
    }

    /* Read the data. */
    if (gz64_read(archive, buf, size) <= 0) {
        fprintf(stderr, "MgzRead: failed to gz64_read().\n");
        gzclose(archive);
        return 0;
    }

    gzclose(archive);
    return size;
}

#undef PRAGMA
#undef PRAGMA_OMP_PARALLEL_FOR

#include "core/data_structures/bitstream.h"

#include <stddef.h>  // NULL
#include <stdint.h>  // uint8_t, int64_t
#include <stdio.h>   // fprintf, stderr
#include <stdlib.h>  // calloc, free
#include <string.h>  // memset

int BitStreamInit(BitStream *stream, int64_t size) {
    stream->num_bytes = (size + 7) / 8;
    stream->stream = (uint8_t *)calloc(stream->num_bytes, sizeof(uint8_t));
    if (stream->stream == NULL) {
        fprintf(stderr, "BitStreamInit: failed to calloc stream\n");
        return 1;
    }

    stream->size = size;
    return 0;
}

void BitStreamDestroy(BitStream *stream) {
    free(stream->stream);
    memset(stream, 0, sizeof(*stream));
}

static int GetByteOffset(int64_t i) { return i % 8; }

static int GetLocalBitOffset(int64_t i) { return i % 8; }

static int SetTo(BitStream *stream, int64_t i, uint8_t value) {
    if (i >= stream->size) return -1;

    int byte_offset = GetByteOffset(i);
    uint8_t *byte_address = stream->stream + byte_offset;

    int local_bit_offset = GetLocalBitOffset(i);
    uint8_t mask = 1 << local_bit_offset;

    *byte_address = (*byte_address & (~mask)) | (value << local_bit_offset);

    return 0;
}

int BitStreamSet(BitStream *stream, int64_t i) {
    return SetTo(stream, i, 1);
}

int BitStreamClear(BitStream *stream, int64_t i) {
    return SetTo(stream, i, 0);
}

int BitStreamGet(const BitStream *stream, int64_t i) {
    if (i >= stream->size) return -1;

    int byte_offset = GetByteOffset(i);
    const uint8_t *byte_address = stream->stream + byte_offset;

    int local_bit_offset = GetLocalBitOffset(i);
    uint8_t mask = 1 << local_bit_offset;

    return (*byte_address) & mask;
}

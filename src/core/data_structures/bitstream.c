/**
 * @file bitstream.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-size bit stream implementation.
 * @version 1.1.0
 * @date 2024-09-05
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

#include "core/data_structures/bitstream.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdint.h>   // uint8_t, int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // calloc, free
#include <string.h>   // memset

int BitStreamInit(BitStream *stream, int64_t size) {
    stream->num_bytes = (size + 7) / 8;  // Round up division.
    stream->stream = (uint8_t *)calloc(stream->num_bytes, sizeof(uint8_t));
    if (stream->stream == NULL) {
        fprintf(stderr, "BitStreamInit: failed to calloc stream\n");
        return 1;
    }

    stream->size = size;
    stream->count = 0;

    return 0;
}

void BitStreamDestroy(BitStream *stream) {
    free(stream->stream);
    memset(stream, 0, sizeof(*stream));
}

static int GetByteOffset(int64_t i) { return i / 8; }

static int GetLocalBitOffset(int64_t i) { return i % 8; }

static int SetTo(BitStream *stream, int64_t i, uint8_t value) {
    if (i >= stream->size) return -1;

    int byte_offset = GetByteOffset(i);
    uint8_t *byte_address = stream->stream + byte_offset;

    int local_bit_offset = GetLocalBitOffset(i);
    uint8_t mask = 1 << local_bit_offset;

    stream->count += value - (bool)((*byte_address) & mask);
    *byte_address = (*byte_address & (~mask)) | (value << local_bit_offset);

    return 0;
}

int BitStreamSet(BitStream *stream, int64_t i) { return SetTo(stream, i, 1); }

int BitStreamClear(BitStream *stream, int64_t i) { return SetTo(stream, i, 0); }

bool BitStreamGet(const BitStream *stream, int64_t i) {
    if (i >= stream->size) return -1;

    int byte_offset = GetByteOffset(i);
    const uint8_t *byte_address = stream->stream + byte_offset;

    int local_bit_offset = GetLocalBitOffset(i);
    uint8_t mask = 1 << local_bit_offset;

    return (*byte_address) & mask;
}

int64_t BitStreamCount(const BitStream *stream) { return stream->count; }

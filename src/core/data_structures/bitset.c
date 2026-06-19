/**
 * @file bitset.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-size bit set implementation.
 * @version 1.0.0
 * @date 2025-06-30
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

#include "core/data_structures/bitset.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "core/gamesman_memory.h"

typedef uint64_t BlockType;
static const int64_t kBitsPerBlock = sizeof(BlockType) * 8;
static const BlockType kOne = 1;

struct Bitset {
    int64_t num_bits; /**< Size of the bit set in number of bits. */
    int64_t count;    /**< Counter for the number of bits set to true. */
    BlockType data[]; /**< Raw data. */
};

static int64_t NumBitsToNumBlocks(int64_t num_bits) {
    return (num_bits + kBitsPerBlock - 1) / kBitsPerBlock;
}

size_t BitsetMemRequired(int64_t num_bits) {
    if (num_bits < 0) num_bits = 0;
    int64_t num_blocks = NumBitsToNumBlocks(num_bits);

    return sizeof(Bitset) + num_blocks * sizeof(BlockType);
}

Bitset *BitsetCreate(int64_t num_bits) {
    if (num_bits < 0) num_bits = 0;
    size_t alloc_size = BitsetMemRequired(num_bits);
    Bitset *ret = (Bitset *)GamesmanCallocWhole(1, alloc_size);
    if (ret != NULL) ret->num_bits = num_bits;

    return ret;
}

void BitsetDestroy(Bitset *bs) { GamesmanFree(bs); }

static int64_t BlockIndex(int64_t bit_index) {
    return bit_index / kBitsPerBlock;
}

static int64_t BitOffset(int64_t bit_index) {
    return bit_index % kBitsPerBlock;
}

bool BitsetSet(Bitset *bs, int64_t i) {
    assert(i >= 0 && i < bs->num_bits);
    int64_t bit_offset = BitOffset(i);
    int64_t block_index = BlockIndex(i);
    BlockType mask = kOne << bit_offset;

    BlockType prev_block = bs->data[block_index];
    bool prev = prev_block & mask;

    bs->data[block_index] |= mask;
    bs->count += !prev;

    return prev;
}

bool BitsetReset(Bitset *bs, int64_t i) {
    assert(i >= 0 && i < bs->num_bits);
    int64_t bit_offset = BitOffset(i);
    int64_t block_index = BlockIndex(i);
    BlockType mask = kOne << bit_offset;

    BlockType prev_block = bs->data[block_index];
    bool prev = prev_block & mask;

    bs->data[block_index] &= ~mask;
    bs->count -= prev;

    return prev;
}

bool BitsetSetTo(Bitset *bs, int64_t i, bool val) {
    assert(i >= 0 && i < bs->num_bits);
    int64_t bit_offset = BitOffset(i);
    int64_t block_index = BlockIndex(i);
    BlockType mask = kOne << bit_offset;
    BlockType vmask = (BlockType)val * mask;
    BlockType prev_block = bs->data[block_index];
    bool prev = prev_block & mask;

    bs->data[block_index] = (prev_block & ~mask) | vmask;
    bs->count += (int)val - (int)prev;

    return prev;
}

bool BitsetTest(const Bitset *bs, int64_t i) {
    assert(i >= 0 && i < bs->num_bits);
    int64_t bit_offset = BitOffset(i);
    int64_t block_index = BlockIndex(i);
    BlockType mask = kOne << bit_offset;

    return bs->data[block_index] & mask;
}

int64_t BitsetCount(const Bitset *bs) { return bs->count; }

size_t BitSetGetSerializedSize(const Bitset *bs) {
    return BitsetMemRequired(bs->num_bits);
}

size_t BitsetSerializeStreaming(const Bitset *bs, size_t offset, void *buf,
                                size_t buf_size) {
    size_t total_bytes = BitSetGetSerializedSize(bs);
    if (offset >= total_bytes) return 0;

    size_t remaining_bytes = total_bytes - offset;
    size_t bytes_to_copy =
        remaining_bytes < buf_size ? remaining_bytes : buf_size;
    memcpy(buf, (char *)bs + offset, bytes_to_copy);

    return bytes_to_copy;
}

Bitset *BitsetDeserializeStreaming(Bitset *bs, size_t offset, const void *in,
                                   size_t in_size) {
    if (bs == NULL) {
        if (offset != 0 || in_size < sizeof(int64_t)) return NULL;

        // Read in the number of bits
        bs = BitsetCreate(*(int64_t *)in);
        offset += sizeof(int64_t);
        in = (const char *)in + offset;
        in_size -= offset;
    }

    memcpy((char *)bs + offset, in, in_size);

    return bs;
}

void *BitsetGetRawData(Bitset *bs) { return (void *)bs; }
